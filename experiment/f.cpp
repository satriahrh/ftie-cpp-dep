// make experiment-build-f && make experiment-run-f
// Corpus-driven experiment: one row of metrics per manifest file, instead
// of a-e.cpp's single-parameter sweeps over random plaintext (it replaces
// the old synthetic f.cpp size sweep, decision 7 / O8). See new-experiment.md for the requirements this implements (D1,D7,M1-M7,O1-O3,O6).
//
// Usage: ./bin/exp-f.o [manifest-csv] [output-csv]
//   defaults: experiment/corpus_manifest.csv, experiment/results/f.csv
//
// Unlike a-e.cpp (which print rows to stdout for a shell script to
// redirect), this program owns its own CSV file I/O, because file-level
// resume requires reading back which files are already done: a run that
// stops partway can be re-invoked with the same args and will skip
// already-completed files (matched by sha256) rather than redoing them.
// BBS and ACM parameters are drawn once on a run's first invocation and
// frozen in a sidecar "<output-csv>.params" file so a resumed run reuses
// the exact same parameters rather than silently mixing two parameter sets
// across rows.
#include "../ftie/ftie.h"
#include "../experiment/manifest.h"
#include "../experiment/sha256.h"
#include "../experiment/tools.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
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
    }
    if (pStr.empty() || qStr.empty() || sStr.empty()) return false;

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

  void write_file_bytes(const std::string & path, const std::vector<uint8_t> & bytes) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!bytes.empty()) file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  }
}

int main(int argc, char const *argv[]) {
  const std::string manifestPath = argc > 1 ? argv[1] : "experiment/corpus_manifest.csv";
  const std::string outputCsvPath = argc > 2 ? argv[2] : "experiment/results/f.csv";
  const std::string paramsPath = outputCsvPath + ".params";

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

    std::random_device generateRandom;

    RunParams params;
    if (!load_params(paramsPath, params)) {
      std::vector<tools::BbsParams> bbsParams = tools::read_bbs_params("experiment/bbs_params.csv");
      const tools::BbsParams & last = bbsParams.back();
      params.bits = last.bits;
      params.p = last.p;
      params.q = last.q;
      params.s = last.s;
      params.acm_a = generateRandom();
      params.acm_b = generateRandom();
      params.acm_n = 50;
      write_params(paramsPath, params, manifestPath);
    }

    // Deprecated-pipeline keystream: a fixed-size random buffer, matching
    // a-e.cpp's existing convention exactly. rt::encrypt indexes this
    // modulo its length, so one buffer generated once per run is correct
    // for every file regardless of plainfile size -- no per-file or
    // per-manifest-max sizing is needed.
    std::vector<uint8_t> keystream(10000000);
    for (size_t i = 0; i < keystream.size(); i++) keystream[i] = generateRandom();

    std::vector<tools::CorpusFile> manifest = tools::read_manifest(manifestPath.c_str());
    std::set<std::string> done = load_done_sha256(outputCsvPath);

    std::ofstream out(outputCsvPath, std::ios::out | std::ios::app);
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
        std::string tag = f.sha256.substr(0, 12);
        const std::string & plainfile1 = f.path;
        std::string plainfile2 = "experiment/dum/f_" + tag + "_plainfile_2";
        std::string cipherimage1 = "experiment/dum/f_" + tag + "_cipherimage_1";
        std::string cipherimage2 = "experiment/dum/f_" + tag + "_cipherimage_2";
        std::string roundtrip1 = "experiment/dum/f_" + tag + "_roundtrip_1";

        // plainfile_2: plainfile_1 with bit 0 of byte 0 flipped (M4; same
        // scheme as a-e.cpp's byte-0 1-vs-3 convention, generalized to a
        // single bit flip since corpus plainfiles don't start at byte 1).
        std::vector<uint8_t> plainBytes1 = read_file_bytes(plainfile1);
        std::vector<uint8_t> plainBytes2 = plainBytes1;
        if (!plainBytes2.empty()) plainBytes2[0] ^= 0x01;
        write_file_bytes(plainfile2, plainBytes2);

        double npcr_dep, npcr_cur, npcr_ch_dep, npcr_ch_cur;
        double uaci_dep, uaci_cur;
        double entropy_dep, entropy_cur;
        double chi2_dep, chi2_cur;
        double avalanche_dep, avalanche_cur;
        std::chrono::duration<double> enc_dep, dec_dep, enc_cur, dec_cur;

        {
          enc_dep = tools::chrono_deprecated_encrypt(keystream, params.acm_a, params.acm_b, params.acm_n, plainfile1.c_str(), cipherimage1.c_str());
          dec_dep = tools::chrono_deprecated_decrypt(keystream, params.acm_a, params.acm_b, params.acm_n, cipherimage1.c_str(), roundtrip1.c_str());
          if (read_file_bytes(roundtrip1) != plainBytes1) {
            throw std::runtime_error("deprecated round-trip mismatch for " + f.file_name);
          }

          ftie::deprecated::encrypt(keystream, params.acm_a, params.acm_b, params.acm_n, plainfile1.c_str(), cipherimage2.c_str());
          npcr_dep = tools::calculate_npcr(cipherimage1.c_str(), cipherimage2.c_str());
          npcr_ch_dep = tools::calculate_npcr_channel(cipherimage1.c_str(), cipherimage2.c_str());
          uaci_dep = tools::calculate_uaci(cipherimage1.c_str(), cipherimage2.c_str());

          entropy_dep = tools::calculate_entropy(cipherimage1.c_str());
          chi2_dep = tools::calculate_chi_square(cipherimage1.c_str());

          ftie::deprecated::encrypt(keystream, params.acm_a, params.acm_b, params.acm_n, plainfile2.c_str(), cipherimage2.c_str());
          avalanche_dep = tools::calculate_avalanche(cipherimage1.c_str(), cipherimage2.c_str());
        }

        {
          enc_cur = tools::chrono_encrypt(params.p, params.q, params.s, params.acm_a, params.acm_b, params.acm_n, plainfile1.c_str(), cipherimage1.c_str());
          dec_cur = tools::chrono_decrypt(params.p, params.q, params.s, params.acm_a, params.acm_b, params.acm_n, cipherimage1.c_str(), roundtrip1.c_str());
          if (read_file_bytes(roundtrip1) != plainBytes1) {
            throw std::runtime_error("current round-trip mismatch for " + f.file_name);
          }

          ftie::encrypt(params.p, params.q, params.s, params.acm_a, params.acm_b, params.acm_n, plainfile1.c_str(), cipherimage2.c_str());
          npcr_cur = tools::calculate_npcr(cipherimage1.c_str(), cipherimage2.c_str());
          npcr_ch_cur = tools::calculate_npcr_channel(cipherimage1.c_str(), cipherimage2.c_str());
          uaci_cur = tools::calculate_uaci(cipherimage1.c_str(), cipherimage2.c_str());

          entropy_cur = tools::calculate_entropy(cipherimage1.c_str());
          chi2_cur = tools::calculate_chi_square(cipherimage1.c_str());

          ftie::encrypt(params.p, params.q, params.s, params.acm_a, params.acm_b, params.acm_n, plainfile2.c_str(), cipherimage2.c_str());
          avalanche_cur = tools::calculate_avalanche(cipherimage1.c_str(), cipherimage2.c_str());
        }

        // Both pipelines pad/pack identically for a given plainfile, so
        // cipherimage dimensions are the same either way -- one read
        // suffices (cipherimage1 here holds the _cur pipeline's output,
        // written last).
        png::image<png::rgb_pixel> dims(cipherimage1.c_str());
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
            << npcr_dep << "," << npcr_cur << "," << npcr_ch_dep << "," << npcr_ch_cur << ","
            << uaci_dep << "," << uaci_cur << ","
            << entropy_dep << "," << entropy_cur << "," << chi2_dep << "," << chi2_cur << ","
            << avalanche_dep << "," << avalanche_cur << ","
            << enc_dep.count() << "," << dec_dep.count() << "," << enc_cur.count() << "," << dec_cur.count() << ","
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
