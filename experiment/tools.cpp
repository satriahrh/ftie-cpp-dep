#include "tools.h"

#include "../ftie/ftie.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>

#include "png++/png.hpp"

namespace tools {
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
      hm = p * log2(1 / p);
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

  std::chrono::duration<double> chrono_encrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  ) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    ftie::encrypt(p, q, s, a, b, n, plainfileFilepath, cipherimageFilepath);
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    return finish - start;
  }

  std::chrono::duration<double> chrono_decrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
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
