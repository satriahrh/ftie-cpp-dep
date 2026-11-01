// make experiment-build-f && make experiment-run-f
// Corpus-driven experiment: one row of metrics per manifest file, instead
// of a-e.cpp's single-parameter sweeps over random plaintext (it replaces
// the old synthetic f.cpp size sweep, decision 7 / O8). See
// new-experiment.md for the requirements this implements (D1,D7,M1-M7,O1-O3,O6)
// and experiment/METHODOLOGY.md §6.
//
// Usage: ./bin/exp-f.o [manifest-csv] [output-csv]
//   defaults: experiment/corpus_manifest.csv, experiment/results/f.csv
//
// Unlike a-e.cpp (which print rows to stdout for a shell script to
// redirect), this program owns its own CSV file I/O, because file-level
// resume requires reading back which files are already done: a run that
// stops partway can be re-invoked with the same args and will skip
// already-completed files (matched by sha256) rather than redoing them.
// The key is drawn once on a run's first invocation and frozen so a
// resumed run never mixes two keys across rows: BBS/ACM parameters in the
// sidecar "<output-csv>.params", and the previous pipeline's random
// keystream in "<output-csv>.keystream" (its SHA-256 recorded in .params).
// The proposed pipeline's running BBS seed starts at the frozen s and is
// carried through the files in manifest order; every row records the seeds
// it used and the next one, so a resumed run continues from the last row.
#include "../experiment/manifest.h"
#include "../experiment/sha256.h"
#include "../experiment/tools.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmpxx.h>

#include "png++/png.hpp"

namespace {
  struct RunParams {
    unsigned int bits;
    mpz_class p, q, s;
    uint16_t acm_a, acm_b, acm_n;
    uint64_t keystream_bytes = 0;
    std::string keystream_sha256;
  };

  // key=value lines; returns false if the sidecar doesn't exist yet.
  bool load_params(const std::string & paramsPath, RunParams & out) {
    std::ifstream file(paramsPath);
    if (!file) return false;

    std::string pStr, qStr, sStr;
    std::string line;
    while (std::getline(file, line)) {
      std::size_t eq = line.find('=');
      if (eq == std::string::npos) continue;
      std::string key = line.substr(0, eq);
      std::string val = line.substr(eq + 1);
      if (key == "bits") out.bits = static_cast<unsigned int>(std::stoul(val));
      else if (key == "p") pStr = val;
      else if (key == "q") qStr = val;
      else if (key == "s") sStr = val;
      else if (key == "acm_a") out.acm_a = static_cast<uint16_t>(std::stoul(val));
      else if (key == "acm_b") out.acm_b = static_cast<uint16_t>(std::stoul(val));
      else if (key == "acm_n") out.acm_n = static_cast<uint16_t>(std::stoul(val));
      else if (key == "keystream_bytes") out.keystream_bytes = std::stoull(val);
      else if (key == "keystream_sha256") out.keystream_sha256 = val;
    }
    if (pStr.empty() || qStr.empty() || sStr.empty()) return false;
    if (out.keystream_sha256.empty()) {
      throw std::runtime_error(paramsPath + " has no keystream_sha256 (written by an older harness); "
        "delete it and the output CSV to start a fresh run");
    }

    out.p = mpz_class(pStr, 10);
    out.q = mpz_class(qStr, 10);
    out.s = mpz_class(sStr, 10);
    return true;
  }

  void write_params(const std::string & paramsPath, const RunParams & params, const std::string & manifestPath) {
    std::ofstream file(paramsPath, std::ios::out | std::ios::trunc);
    file << "bits=" << params.bits << "\n";
    file << "p=" << params.p.get_str(10) << "\n";
    file << "q=" << params.q.get_str(10) << "\n";
    file << "s=" << params.s.get_str(10) << "\n";
    file << "acm_a=" << params.acm_a << "\n";
    file << "acm_b=" << params.acm_b << "\n";
    file << "acm_n=" << params.acm_n << "\n";
    file << "keystream_bytes=" << params.keystream_bytes << "\n";
    file << "keystream_sha256=" << params.keystream_sha256 << "\n";
    file << "seed_chain=per-run" << "\n";
    file << "timing_repeats=" << tools::TIMING_REPEATS << "\n";
    file << "manifest=" << manifestPath << "\n";
  }

  std::vector<std::string> split_csv_line(const std::string & line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) fields.push_back(field);
    return fields;
  }

  std::size_t column_index(const std::vector<std::string> & columns, const std::string & name) {
    return std::find(columns.begin(), columns.end(), name) - columns.begin();
  }

  // What a resumed run needs from the rows already written: the finished
  // files (by sha256) and the seed chain's two ends. Every row must be
  // complete (all fields, newline-terminated), since a torn last row would
  // otherwise hand a truncated seed to the next file.
  struct DoneRows {
    std::set<std::string> sha256;
    std::string first_seed_c1, last_seed_next;
  };

  DoneRows load_done_rows(const std::string & outputCsvPath, const std::string & header) {
    DoneRows done;
    std::ifstream file(outputCsvPath, std::ios::in | std::ios::binary);
    if (!file) return done;

    const std::vector<std::string> columns = split_csv_line(header);
    const std::size_t sha256Col = column_index(columns, "sha256");
    const std::size_t seedC1Col = column_index(columns, "seed_c1_proposed");
    const std::size_t seedNextCol = column_index(columns, "seed_next_proposed");

    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!contents.empty() && contents.back() != '\n') {
      throw std::runtime_error(outputCsvPath + " ends in an incomplete row; remove that last line and re-run");
    }
    std::stringstream lines(contents);
    std::string line;
    std::getline(lines, line); // header
    while (std::getline(lines, line)) {
      if (line.empty()) continue;
      std::vector<std::string> fields = split_csv_line(line);
      if (fields.size() != columns.size()) {
        throw std::runtime_error(outputCsvPath + " has a row with " + std::to_string(fields.size()) +
          " fields instead of " + std::to_string(columns.size()));
      }
      done.sha256.insert(fields[sha256Col]);
      if (done.first_seed_c1.empty()) done.first_seed_c1 = fields[seedC1Col];
      done.last_seed_next = fields[seedNextCol];
    }
    return done;
  }

  std::vector<uint8_t> read_file_bytes(const std::string & path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("could not open " + path);
    std::streamsize size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (size > 0) file.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
  }
}

int main(int argc, char const *argv[]) {
  const std::string manifestPath = argc > 1 ? argv[1] : "experiment/corpus_manifest.csv";
  const std::string outputCsvPath = argc > 2 ? argv[2] : "experiment/results/f.csv";
  const std::string paramsPath = outputCsvPath + ".params";
  const std::string keystreamPath = outputCsvPath + ".keystream";

  try {
    std::filesystem::create_directories("experiment/dum");
    std::filesystem::path outputPath(outputCsvPath);
    if (outputPath.has_parent_path()) std::filesystem::create_directories(outputPath.parent_path());

    const std::string header =
      "type,subset,file_name,sha256,size_bytes,width,height,"
      "entropy_plain,chi2_plain,"
      "npcr_previous,npcr_proposed,npcr_ch_previous,npcr_ch_proposed,uaci_previous,uaci_proposed,"
      "entropy_previous,entropy_proposed,chi2_previous,chi2_proposed,"
      "avalanche_previous,avalanche_proposed,"
      "enc_previous_s,dec_previous_s,enc_proposed_s,dec_proposed_s,"
      "npcr_crit_previous,npcr_crit_proposed,uaci_crit_low_previous,uaci_crit_high_previous,"
      "uaci_crit_low_proposed,uaci_crit_high_proposed,entropy_crit,"
      "seed_c1_proposed,seed_c2_proposed,seed_c3_proposed,seed_next_proposed";

    // Refuse to resume into a CSV with a different schema -- notably the
    // old synthetic f.cpp's size-sweep f.csv (O7: keep it, don't append
    // corpus rows under its header). Checked before the params sidecar is
    // written, so a refused run leaves nothing behind.
    bool needsHeader;
    {
      std::ifstream existing(outputCsvPath);
      needsHeader = !existing || existing.peek() == std::ifstream::traits_type::eof();
      if (!needsHeader) {
        std::string existingHeader;
        std::getline(existing, existingHeader);
        if (existingHeader != header) {
          throw std::runtime_error(outputCsvPath + " exists with a different header (old size-sweep results?); "
            "move it aside or pass a different output path");
        }
      }
    }

    std::vector<tools::CorpusFile> manifest = tools::read_manifest(manifestPath.c_str());

    RunParams params;
    std::vector<uint8_t> keystream;
    if (load_params(paramsPath, params)) {
      keystream = read_file_bytes(keystreamPath);
      if (keystream.size() != params.keystream_bytes ||
          tools::sha256_bytes(keystream.data(), keystream.size()) != params.keystream_sha256) {
        throw std::runtime_error(keystreamPath + " does not match keystream_sha256 in " + paramsPath +
          "; restore it, or delete the params, keystream and output CSV to start a fresh run");
      }
    } else {
      const tools::BbsParams last = tools::read_bbs_params("experiment/bbs_params.csv").back();
      params.bits = last.bits;
      params.p = last.p;
      params.q = last.q;
      params.s = last.s;
      params.acm_a = tools::random_acm_parameter();
      params.acm_b = tools::random_acm_parameter();
      params.acm_n = 50;

      // Previous-pipeline keystream: rt::encrypt indexes it modulo its
      // length, so size it past the largest padded plainfile (padding adds
      // well under 64 KiB at 10 MB) so it never wraps within a file; at
      // least 10,000,000 bytes, the a-e convention.
      uint64_t largest = 0;
      for (const tools::CorpusFile & f : manifest) largest = std::max(largest, f.size_bytes);
      keystream = tools::random_bytes(std::max<uint64_t>(10000000, largest + 65536));
      tools::write_bytes(keystreamPath, keystream);
      params.keystream_bytes = keystream.size();
      params.keystream_sha256 = tools::sha256_bytes(keystream.data(), keystream.size());
      write_params(paramsPath, params, manifestPath);
    }

    DoneRows done = load_done_rows(outputCsvPath, header);

    // Running seed: the frozen s on a fresh run, else the next seed recorded
    // by the last finished file.
    mpz_class seed = params.s;
    if (!done.last_seed_next.empty()) {
      if (done.first_seed_c1 != params.s.get_str(10)) {
        throw std::runtime_error(outputCsvPath + "'s first row was not encrypted from s in " + paramsPath +
          "; the CSV and the params belong to different runs");
      }
      seed = mpz_class(done.last_seed_next, 10);
    }

    std::ofstream out(outputCsvPath, std::ios::out | std::ios::app);
    out << std::setprecision(tools::CSV_PRECISION);
    if (needsHeader) {
      out << header << "\n";
      out.flush();
    }

    for (const tools::CorpusFile & f : manifest) {
      if (done.sha256.count(f.sha256)) {
        std::cerr << "skip (already done): " << f.file_name << " (" << f.sha256 << ")\n";
        continue;
      }

      try {
        const std::string & plainfile1 = f.path;

        // D7: the file on disk must be the one the manifest describes.
        uint64_t actualSize = std::filesystem::file_size(plainfile1);
        if (actualSize != f.size_bytes) {
          throw std::runtime_error("size " + std::to_string(actualSize) + " != manifest " + std::to_string(f.size_bytes));
        }
        if (tools::sha256_file(plainfile1.c_str()) != f.sha256) {
          throw std::runtime_error("sha256 does not match the manifest");
        }

        std::string dum = "experiment/dum/f_" + f.sha256.substr(0, 12);
        const tools::ScratchPaths paths = {
          dum + "_plainfile_2", dum + "_cipherimage_1", dum + "_cipherimage_2", dum + "_roundtrip_1"
        };
        tools::write_bit1_flipped(plainfile1.c_str(), paths.plain2.c_str());

        // fileSeed advances only into seed once the row is written, so a
        // FAILED file leaves the chain where the last row ended, as a resumed
        // run would find it.
        mpz_class fileSeed = seed;
        tools::ProposedSeeds seeds;
        tools::PipelineResult prev = tools::measure_previous(keystream, params.acm_a, params.acm_b, params.acm_n, plainfile1, paths);
        tools::PipelineResult prop = tools::measure_proposed(params.p, params.q, fileSeed, params.acm_a, params.acm_b, params.acm_n, plainfile1, paths, seeds);

        // Both pipelines pad/pack identically for a given plainfile, so
        // cipherimage dimensions are the same either way -- one read
        // suffices (cipherimage1 here holds the proposed pipeline's output,
        // written last).
        png::image<png::rgb_pixel> dims(paths.cipher1.c_str());
        uint32_t width = dims.get_width();
        uint32_t height = dims.get_height();

        double entropy_plain = tools::calculate_entropy_bytes(plainfile1.c_str());
        double chi2_plain = tools::calculate_chi_square_bytes(plainfile1.c_str());

        double npcr_crit_prev = tools::npcr_critical_value(width, height, tools::Z_ONE_SIDED_ALPHA_05, true);
        double npcr_crit_prop = tools::npcr_critical_value(width, height, tools::Z_ONE_SIDED_ALPHA_05, false);
        std::pair<double, double> uaci_crit_prev = tools::uaci_critical_range(width, height, tools::Z_TWO_SIDED_ALPHA_05, true);
        std::pair<double, double> uaci_crit_prop = tools::uaci_critical_range(width, height, tools::Z_TWO_SIDED_ALPHA_05, false);
        double entropy_crit = tools::entropy_critical_value(width, height, tools::CHI_SQUARE_CRITICAL_DF255_ALPHA_05);

        out << f.type << "," << f.subset << "," << f.file_name << "," << f.sha256 << "," << f.size_bytes << ","
            << width << "," << height << ","
            << entropy_plain << "," << chi2_plain << ","
            << prev.npcr << "," << prop.npcr << "," << prev.npcr_ch << "," << prop.npcr_ch << ","
            << prev.uaci << "," << prop.uaci << ","
            << prev.entropy << "," << prop.entropy << "," << prev.chi2 << "," << prop.chi2 << ","
            << prev.avalanche << "," << prop.avalanche << ","
            << prev.enc_s << "," << prev.dec_s << "," << prop.enc_s << "," << prop.dec_s << ","
            << npcr_crit_prev << "," << npcr_crit_prop << ","
            << uaci_crit_prev.first << "," << uaci_crit_prev.second << ","
            << uaci_crit_prop.first << "," << uaci_crit_prop.second << "," << entropy_crit << ","
            << seeds.c1.get_str(10) << "," << seeds.c2.get_str(10) << ","
            << seeds.c3.get_str(10) << "," << seeds.next.get_str(10) << "\n";
        out.flush();
        seed = fileSeed;

        std::cerr << "done: " << f.file_name << " (" << f.sha256 << ")\n";
      } catch (const std::exception & e) {
        std::cerr << "FAILED " << f.file_name << ": " << e.what() << " -- skipping, will retry on next run\n";
      } catch (const char * e) {
        std::cerr << "FAILED " << f.file_name << ": " << e << " -- skipping, will retry on next run\n";
      }
    }
  } catch (const char * msg) {
    std::cerr << msg << '\n';
    return 1;
  } catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}
