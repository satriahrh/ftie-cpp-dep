#include "tools.h"

#include "../ftie/ftie.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
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
    return params;
  }

  double calculate_avalanche(
    const char * path_1,
    const char * path_2
  ) {
    std::ifstream fst(path_1, std::ios::in | std::ios::binary);
    std::ifstream snd(path_2, std::ios::in | std::ios::binary);
    uint64_t n = 0;
    uint64_t diff = 0;
    char cfst;
    char csnd;
    while (fst.get(cfst) && snd.get(csnd)) {
      for (int i = 7; i >= 0; i--) {
        bool a = ((cfst >> i) & 1);
        bool b = ((csnd >> i) & 1);
        if (a != b)
        diff++;
        n++;
      }
    }
    double n_double = n;
    return diff / n_double;
  }

  double calculate_entropy(
    const char * cipherimageFilepath
  ) {
    png::image<png::rgb_pixel> image(cipherimageFilepath);

    uint_fast16_t N = image.get_height();
    std::vector<uint_fast16_t> sum(256);
    for (uint_fast16_t x = 0; x < N; x++) {
      for (uint_fast16_t y = 0; y < N; y++) {
        png::rgb_pixel px = image[y][x];
        sum[px.red]++;
        sum[px.green]++;
        sum[px.blue]++;
      }
    }
    uint_fast32_t n = N * N * 3;

    double hm = 0;
    for (int i = 0; i < 256; i++) {
      double p = sum[i] / (n * 1.0);
      hm += p * log2(1 / p);
    }
    return hm;
  }

  double calculate_npcr(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);

    uint_fast16_t N = image_1.get_height();
    uint_fast32_t sum = 0;
    for (uint_fast16_t x = 0; x < N; x++) {
      for (uint_fast16_t y = 0; y < N; y++) {
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
    double n = N * N;

    return sum / n;
  }

  double calculate_npcr_channel(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);

    uint_fast16_t N = image_1.get_height();
    uint_fast32_t diffRed = 0;
    uint_fast32_t diffGreen = 0;
    uint_fast32_t diffBlue = 0;
    for (uint_fast16_t x = 0; x < N; x++) {
      for (uint_fast16_t y = 0; y < N; y++) {
        png::rgb_pixel px_1 = image_1[y][x];
        png::rgb_pixel px_2 = image_2[y][x];
        if (px_1.red != px_2.red) diffRed++;
        if (px_1.green != px_2.green) diffGreen++;
        if (px_1.blue != px_2.blue) diffBlue++;
      }
    }
    double n = N * N;

    return ((diffRed + diffGreen + diffBlue) / 3.0) / n;
  }

  double calculate_uaci(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  ) {
    png::image<png::rgb_pixel> image_1(cipherimage1Filepath);
    png::image<png::rgb_pixel> image_2(cipherimage2Filepath);

    uint_fast16_t W = image_1.get_width();
    uint_fast16_t H = image_1.get_height();
    double sum = 0;
    for (uint_fast16_t x = 0; x < W; x++) {
      for (uint_fast16_t y = 0; y < H; y++) {
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
    png::image<png::rgb_pixel> image(cipherimageFilepath);

    uint_fast16_t N = image.get_height();
    std::vector<uint64_t> sum(256);
    for (uint_fast16_t x = 0; x < N; x++) {
      for (uint_fast16_t y = 0; y < N; y++) {
        png::rgb_pixel px = image[y][x];
        sum[px.red]++;
        sum[px.green]++;
        sum[px.blue]++;
      }
    }
    uint64_t n = uint64_t(N) * N * 3;

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
    double F = 3.0 * width * height;
    double mu = 255.0 / 256.0;
    return mu - z * std::sqrt(mu * (1.0 - mu) / F);
  }

  std::pair<double, double> uaci_critical_range(uint32_t width, uint32_t height, double z) {
    double F = 3.0 * width * height;
    double mu = 33.4635;
    double sigma2 = 0.05599;
    double half = z * std::sqrt(sigma2 / F);
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
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::deprecated::encrypt(keystream, a, b, n, plainfileFilepath, cipherimageFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }

  std::chrono::duration<double> chrono_deprecated_decrypt(
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::deprecated::decrypt(keystream, a, b, n, cipherimageFilepath, plainfileFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }
}
