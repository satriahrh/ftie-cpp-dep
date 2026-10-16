#include "tools.h"

#include "../ftie/ftie.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "png++/png.hpp"

namespace tools {
  namespace {
    std::vector<uint64_t> histogram_from_file(const char * filepath) {
      std::ifstream file(filepath, std::ios::in | std::ios::binary);
      std::vector<uint64_t> sum(256);
      char c;
      while (file.get(c)) {
        sum[static_cast<unsigned char>(c)]++;
      }
      return sum;
    }

    // Pooled R+G+B histogram of a cipherimage's decoded pixels.
    std::vector<uint64_t> histogram_from_image(const char * cipherimageFilepath, uint64_t & n) {
      png::image<png::rgb_pixel> image(cipherimageFilepath);
      uint32_t W = image.get_width();
      uint32_t H = image.get_height();
      std::vector<uint64_t> sum(256);
      for (uint32_t y = 0; y < H; y++) {
        for (uint32_t x = 0; x < W; x++) {
          png::rgb_pixel px = image[y][x];
          sum[px.red]++;
          sum[px.green]++;
          sum[px.blue]++;
        }
      }
      n = uint64_t(W) * H * 3;
      return sum;
    }

    double entropy_from_histogram(const std::vector<uint64_t> & sum, uint64_t n) {
      double hm = 0;
      for (int i = 0; i < 256; i++) {
        double p = sum[i] / (n * 1.0);
        if (p > 0) hm += p * log2(1 / p);
      }
      return hm;
    }

    double chi_square_from_histogram(const std::vector<uint64_t> & sum, uint64_t n) {
      double expected = n / 256.0;
      double chi2 = 0;
      for (int i = 0; i < 256; i++) {
        double diff = sum[i] - expected;
        chi2 += (diff * diff) / expected;
      }
      return chi2;
    }

    // Both images must share dimensions for any pairwise metric.
    void check_same_size(const png::image<png::rgb_pixel> & i1, const png::image<png::rgb_pixel> & i2) {
      if (i1.get_width() != i2.get_width() || i1.get_height() != i2.get_height()) {
        throw std::runtime_error("cipherimages differ in size");
      }
    }

    double median(std::vector<double> values) {
      std::sort(values.begin(), values.end());
      size_t mid = values.size() / 2;
      if (values.size() % 2 == 1) return values[mid];
      return (values[mid - 1] + values[mid]) / 2.0;
    }

    // Shared tail of measure_current/measure_deprecated: cipherimage1 holds
    // the last timed Enc(P1), cipherimage2 a second Enc(P1).
    void fill_pair_metrics(PipelineResult & result, const ScratchPaths & paths) {
      result.npcr = calculate_npcr(paths.cipher1.c_str(), paths.cipher2.c_str());
      result.npcr_ch = calculate_npcr_channel(paths.cipher1.c_str(), paths.cipher2.c_str());
      result.uaci = calculate_uaci(paths.cipher1.c_str(), paths.cipher2.c_str());
      result.entropy = calculate_entropy(paths.cipher1.c_str());
      result.chi2 = calculate_chi_square(paths.cipher1.c_str());
    }
  }

  std::vector<BbsParams> read_bbs_params(const char* csvFilepath) {
    std::ifstream file(csvFilepath);
    std::vector<BbsParams> params;

    std::string line;
    std::getline(file, line); // header
    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string bitsStr, pStr, qStr, sStr;
      std::getline(ss, bitsStr, ',');
      std::getline(ss, pStr, ',');
      std::getline(ss, qStr, ',');
      std::getline(ss, sStr, ',');

      BbsParams entry;
      entry.bits = static_cast<unsigned int>(std::stoul(bitsStr));
      entry.p = mpz_class(pStr, 10);
      entry.q = mpz_class(qStr, 10);
      entry.s = mpz_class(sStr, 10);
      params.push_back(entry);
    }
    if (params.empty()) {
      throw std::runtime_error(std::string("no BBS parameters in ") + csvFilepath + " (run make experiment-gen-params)");
    }
    return params;
  }

  PipelineResult measure_current(
    const mpz_class & p, const mpz_class & q, const mpz_class & s,
    uint16_t a, uint16_t b, uint16_t n,
    const std::string & plainfile1, const ScratchPaths & paths
  ) {
    PipelineResult result;
    std::vector<double> enc, dec;
    for (int i = 0; i < TIMING_REPEATS; i++) {
      enc.push_back(chrono_encrypt(p, q, s, a, b, n, plainfile1.c_str(), paths.cipher1.c_str()).count());
      dec.push_back(chrono_decrypt(p, q, s, a, b, n, paths.cipher1.c_str(), paths.roundtrip.c_str()).count());
      if (!files_equal(paths.roundtrip.c_str(), plainfile1.c_str())) {
        throw std::runtime_error("current pipeline round-trip mismatch for " + plainfile1);
      }
    }
    result.enc_s = median(enc);
    result.dec_s = median(dec);

    ftie::encrypt(p, q, s, a, b, n, plainfile1.c_str(), paths.cipher2.c_str());
    fill_pair_metrics(result, paths);

    ftie::encrypt(p, q, s, a, b, n, paths.plain2.c_str(), paths.cipher2.c_str());
    result.avalanche = calculate_avalanche(paths.cipher1.c_str(), paths.cipher2.c_str());
    return result;
  }

  PipelineResult measure_deprecated(
    const std::vector<uint8_t> & keystream,
    uint16_t a, uint16_t b, uint16_t n,
    const std::string & plainfile1, const ScratchPaths & paths
  ) {
    PipelineResult result;
    std::vector<double> enc, dec;
    for (int i = 0; i < TIMING_REPEATS; i++) {
      enc.push_back(chrono_deprecated_encrypt(keystream, a, b, n, plainfile1.c_str(), paths.cipher1.c_str()).count());
      dec.push_back(chrono_deprecated_decrypt(keystream, a, b, n, paths.cipher1.c_str(), paths.roundtrip.c_str()).count());
      if (!files_equal(paths.roundtrip.c_str(), plainfile1.c_str())) {
        throw std::runtime_error("deprecated pipeline round-trip mismatch for " + plainfile1);
      }
    }
    result.enc_s = median(enc);
    result.dec_s = median(dec);

    ftie::deprecated::encrypt(keystream, a, b, n, plainfile1.c_str(), paths.cipher2.c_str());
    fill_pair_metrics(result, paths);

    ftie::deprecated::encrypt(keystream, a, b, n, paths.plain2.c_str(), paths.cipher2.c_str());
    result.avalanche = calculate_avalanche(paths.cipher1.c_str(), paths.cipher2.c_str());
    return result;
  }

  void write_bit1_flipped(const char * srcFilepath, const char * dstFilepath) {
    std::ifstream src(srcFilepath, std::ios::in | std::ios::binary);
    if (!src) throw std::runtime_error(std::string("could not open ") + srcFilepath);
    std::vector<char> bytes((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    if (!bytes.empty()) bytes[0] ^= 0x02;
    std::ofstream dst(dstFilepath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!bytes.empty()) dst.write(bytes.data(), bytes.size());
    if (!dst) throw std::runtime_error(std::string("could not write ") + dstFilepath);
  }

  std::vector<uint8_t> random_bytes(size_t n) {
    std::random_device generateRandom;
    std::vector<uint8_t> bytes(n);
    for (size_t i = 0; i < n; i++) bytes[i] = generateRandom();
    return bytes;
  }

  void write_bytes(const std::string & filepath, const std::vector<uint8_t> & bytes) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!bytes.empty()) file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!file) throw std::runtime_error("could not write " + filepath);
  }

  uint16_t random_acm_parameter() {
    std::random_device generateRandom;
    uint16_t value;
    do {
      value = generateRandom();
    } while (value == 0);
    return value;
  }

  const char * const SWEEP_HEADER =
    "prime_bits,acm_a,acm_b,acm_n,"
    "npcr_dep,npcr_ch_dep,uaci_dep,entropy_dep,chi2_dep,avalanche_dep,enc_dep_s,dec_dep_s,"
    "npcr_cur,npcr_ch_cur,uaci_cur,entropy_cur,chi2_cur,avalanche_cur,enc_cur_s,dec_cur_s";

  void write_sweep_row(
    std::ostream & out, unsigned int primeBits, uint16_t a, uint16_t b, uint16_t n,
    const PipelineResult & dep, const PipelineResult & cur
  ) {
    out << primeBits << "," << a << "," << b << "," << n;
    for (const PipelineResult * r : { &dep, &cur }) {
      out << "," << r->npcr << "," << r->npcr_ch << "," << r->uaci << "," << r->entropy
          << "," << r->chi2 << "," << r->avalanche << "," << r->enc_s << "," << r->dec_s;
    }
  }

  bool files_equal(const char * path1, const char * path2) {
    std::ifstream f1(path1, std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream f2(path2, std::ios::in | std::ios::binary | std::ios::ate);
    if (!f1 || !f2 || f1.tellg() != f2.tellg()) return false;
    f1.seekg(0);
    f2.seekg(0);
    return std::equal(
      std::istreambuf_iterator<char>(f1), std::istreambuf_iterator<char>(),
      std::istreambuf_iterator<char>(f2)
    );
  }

  double calculate_avalanche(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);
    check_same_size(image_1, image_2);

    uint32_t W = image_1.get_width();
    uint32_t H = image_1.get_height();
    uint64_t diff = 0;
    for (uint32_t y = 0; y < H; y++) {
      for (uint32_t x = 0; x < W; x++) {
        png::rgb_pixel px_1 = image_1[y][x];
        png::rgb_pixel px_2 = image_2[y][x];
        diff += std::bitset<8>(px_1.red ^ px_2.red).count();
        diff += std::bitset<8>(px_1.green ^ px_2.green).count();
        diff += std::bitset<8>(px_1.blue ^ px_2.blue).count();
      }
    }
    double bits = 24.0 * W * H;
    return diff / bits;
  }

  double calculate_entropy(
    const char * cipherimageFilepath
  ) {
    uint64_t n;
    std::vector<uint64_t> sum = histogram_from_image(cipherimageFilepath, n);
    return entropy_from_histogram(sum, n);
  }

  double calculate_npcr(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);
    check_same_size(image_1, image_2);

    uint32_t W = image_1.get_width();
    uint32_t H = image_1.get_height();
    uint64_t sum = 0;
    for (uint32_t y = 0; y < H; y++) {
      for (uint32_t x = 0; x < W; x++) {
        png::rgb_pixel px_1 = image_1[y][x];
        png::rgb_pixel px_2 = image_2[y][x];
        if (!(
          (px_1.red == px_2.red) &&
          (px_1.green == px_2.green) &&
          (px_1.blue == px_2.blue)
        )) {
          sum++;
        }
      }
    }
    double n = double(W) * H;

    return sum / n;
  }

  double calculate_npcr_channel(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);
    check_same_size(image_1, image_2);

    uint32_t W = image_1.get_width();
    uint32_t H = image_1.get_height();
    uint64_t diffRed = 0;
    uint64_t diffGreen = 0;
    uint64_t diffBlue = 0;
    for (uint32_t y = 0; y < H; y++) {
      for (uint32_t x = 0; x < W; x++) {
        png::rgb_pixel px_1 = image_1[y][x];
        png::rgb_pixel px_2 = image_2[y][x];
        if (px_1.red != px_2.red) diffRed++;
        if (px_1.green != px_2.green) diffGreen++;
        if (px_1.blue != px_2.blue) diffBlue++;
      }
    }
    double n = double(W) * H;

    return ((diffRed + diffGreen + diffBlue) / 3.0) / n;
  }

  double calculate_uaci(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);
    check_same_size(image_1, image_2);

    uint32_t W = image_1.get_width();
    uint32_t H = image_1.get_height();
    uint64_t sum = 0;
    for (uint32_t y = 0; y < H; y++) {
      for (uint32_t x = 0; x < W; x++) {
        png::rgb_pixel px_1 = image_1[y][x];
        png::rgb_pixel px_2 = image_2[y][x];
        sum += std::abs(px_1.red - px_2.red);
        sum += std::abs(px_1.green - px_2.green);
        sum += std::abs(px_1.blue - px_2.blue);
      }
    }
    double n = 3.0 * W * H;

    return (sum / 255.0) / n * 100.0;
  }

  double calculate_chi_square(
    const char * cipherimageFilepath
  ) {
    uint64_t n;
    std::vector<uint64_t> sum = histogram_from_image(cipherimageFilepath, n);
    return chi_square_from_histogram(sum, n);
  }

  double calculate_entropy_bytes(
    const char * filepath
  ) {
    std::vector<uint64_t> sum = histogram_from_file(filepath);
    uint64_t n = 0;
    for (uint64_t count : sum) n += count;
    return entropy_from_histogram(sum, n);
  }

  double calculate_chi_square_bytes(
    const char * filepath
  ) {
    std::vector<uint64_t> sum = histogram_from_file(filepath);
    uint64_t n = 0;
    for (uint64_t count : sum) n += count;
    return chi_square_from_histogram(sum, n);
  }

  double npcr_critical_value(uint32_t width, uint32_t height, double z) {
    double pairs = 1.5 * width * height;
    double mu = 255.0 / 256.0;
    return mu - z * std::sqrt(mu * (1.0 - mu) / pairs);
  }

  std::pair<double, double> uaci_critical_range(uint32_t width, uint32_t height, double z) {
    double pairs = 1.5 * width * height;
    double mu = 100.0 * 257.0 / 768.0;
    double half = z * 100.0 * std::sqrt(UACI_PAIR_VARIANCE / pairs);
    return { mu - half, mu + half };
  }

  double entropy_critical_value(uint32_t width, uint32_t height, double chiSquareCritical) {
    double N = 3.0 * width * height;
    return 8.0 - chiSquareCritical / (2.0 * N * std::log(2.0));
  }

  std::chrono::duration<double> chrono_encrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::encrypt(p, q, s, a, b, n, plainfileFilepath, cipherimageFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }

  std::chrono::duration<double> chrono_decrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::decrypt(p, q, s, a, b, n, cipherimageFilepath, plainfileFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }

  std::chrono::duration<double> chrono_deprecated_encrypt(
    const std::vector<uint8_t> & keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::deprecated::encrypt(keystream, a, b, n, plainfileFilepath, cipherimageFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }

  std::chrono::duration<double> chrono_deprecated_decrypt(
    const std::vector<uint8_t> & keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::deprecated::decrypt(keystream, a, b, n, cipherimageFilepath, plainfileFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }
}
