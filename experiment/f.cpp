// make experiment-build-f && make experiment-run-f
// Corpus-driven experiment: one row of metrics per manifest file, instead
// of a-e.cpp's single-parameter sweeps over random plaintext (it replaces
// the old synthetic f.cpp size sweep, decision 7 / O8). See
// new-experiment.md for the requirements this implements (D1,D7,M1-M7,O1-O3,O6)
// and experiment/METHODOLOGY.md §7.
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
// sidecar "<output-csv>.params", and the deprecated pipeline's random
// keystream in "<output-csv>.keystream" (its SHA-256 recorded in .params).
#include "../experiment/manifest.h"
#include "../experiment/sha256.h"
#include "../experiment/tools.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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
    file << "timing_repeats=" << tools::TIMING_REPEATS << "\n";
    file << "manifest=" << manifestPath << "\n";
  }

  // sha256 is CSV column index 3 (0-based): type,subset,file_name,sha256,...
  std::set<std::string> load_done_sha256(const std::string & outputCsvPath) {
    std::set<std::string> done;
    std::ifstream file(outputCsvPath);
    if (!file) return done;

    std::string line;
    std::getline(file, line); // header
    while (std::getline(file, line)) {
      if (line.empty()) continue;
      std::stringstream ss(line);
      std::string field;
      for (int i = 0; i <= 3; i++) std::getline(ss, field, ',');
      done.insert(field);
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
      "npcr_dep,npcr_cur,npcr_ch_dep,npcr_ch_cur,uaci_dep,uaci_cur,"
      "entropy_dep,entropy_cur,chi2_dep,chi2_cur,"
      "avalanche_dep,avalanche_cur,"
      "enc_dep_s,dec_dep_s,enc_cur_s,dec_cur_s,"
      "npcr_crit,uaci_crit_low,uaci_crit_high,entropy_crit";

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

      // Deprecated-pipeline keystream: rt::encrypt indexes it modulo its
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

    std::set<std::string> done = load_done_sha256(outputCsvPath);

    std::ofstream out(outputCsvPath, std::ios::out | std::ios::app);
    out << std::setprecision(tools::CSV_PRECISION);
    if (needsHeader) {
      out << header << "\n";
      out.flush();
    }

    for (const tools::CorpusFile & f : manifest) {
      if (done.count(f.sha256)) {
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

        tools::PipelineResult dep = tools::measure_deprecated(keystream, params.acm_a, params.acm_b, params.acm_n, plainfile1, paths);
        tools::PipelineResult cur = tools::measure_current(params.p, params.q, params.s, params.acm_a, params.acm_b, params.acm_n, plainfile1, paths);

        // Both pipelines pad/pack identically for a given plainfile, so
        // cipherimage dimensions are the same either way -- one read
        // suffices (cipherimage1 here holds the _cur pipeline's output,
        // written last).
        png::image<png::rgb_pixel> dims(paths.cipher1.c_str());
        uint32_t width = dims.get_width();
        uint32_t height = dims.get_height();

        double entropy_plain = tools::calculate_entropy_bytes(plainfile1.c_str());
        double chi2_plain = tools::calculate_chi_square_bytes(plainfile1.c_str());

        double npcr_crit = tools::npcr_critical_value(width, height, tools::Z_ONE_SIDED_ALPHA_05);
        std::pair<double, double> uaci_crit = tools::uaci_critical_range(width, height, tools::Z_TWO_SIDED_ALPHA_05);
        double entropy_crit = tools::entropy_critical_value(width, height, tools::CHI_SQUARE_CRITICAL_DF255_ALPHA_05);

        out << f.type << "," << f.subset << "," << f.file_name << "," << f.sha256 << "," << f.size_bytes << ","
            << width << "," << height << ","
            << entropy_plain << "," << chi2_plain << ","
            << dep.npcr << "," << cur.npcr << "," << dep.npcr_ch << "," << cur.npcr_ch << ","
            << dep.uaci << "," << cur.uaci << ","
            << dep.entropy << "," << cur.entropy << "," << dep.chi2 << "," << cur.chi2 << ","
            << dep.avalanche << "," << cur.avalanche << ","
            << dep.enc_s << "," << dep.dec_s << "," << cur.enc_s << "," << cur.dec_s << ","
            << npcr_crit << "," << uaci_crit.first << "," << uaci_crit.second << "," << entropy_crit << "\n";
        out.flush();

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
