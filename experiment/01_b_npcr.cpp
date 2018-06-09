#include "../ftie/ftie.h"

#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <iostream>


struct npcr {
  uint_fast32_t sum;
  uint_fast32_t n;
};

npcr calculate_npcr(
  png::image<png::rgb_pixel> image_1,
  png::image<png::rgb_pixel> image_2
) {
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
  uint_fast32_t n = N * N;

  npcr ret;
  ret.sum = sum;
  ret.n = n;

  return ret;
}
// g++ -o bin/01-b -std=c++17 ftie/*.h ftie/*.cpp experiment/01_b_npcr.cpp `libpng-config --ldflags`
int main(int argc, char const *argv[]) {
  try {
    std::random_device generateRandom;

    // keystream
    std::vector<uint8_t> keystream(10000000);
    for (int i = 0; i < keystream.size(); i++)
    keystream[i] = generateRandom();

    // acm
    uint_fast16_t a = generateRandom();
    uint_fast16_t b = generateRandom();
    uint_fast16_t n = generateRandom() % 100;

    // bbs
    uint16_t p = 55603;
    uint16_t q = 57467;
    uint32_t s = 1369318511;

    std::cout << "bytes | deprecated | proposed | N*N" << '\n';
    std::cout << "--- | --- | --- | ---" << '\n';

    for (int ip = 1000000; ip <= 10000000; ip += 1000000) {
      // plainfile
      std::vector<uint8_t> bytes(ip);
      for (int i = 0; i < bytes.size(); i++)
      bytes[i] = generateRandom();
      std::ofstream outfile("experiment/dum/plainfile", std::ios::out | std::ios::binary);
      outfile.write((const char*)&bytes[0], bytes.size());
      outfile.close();
      npcr npcr_1;
      npcr npcr_2;
      {
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile", "experiment/dum/1.png");
        png::image<png::rgb_pixel> cp_1("experiment/dum/1.png");
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile", "experiment/dum/2.png");
        png::image<png::rgb_pixel> cp_2("experiment/dum/2.png");
        npcr_1 = calculate_npcr(cp_1, cp_2);
      } {
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile", "experiment/dum/1.png");
        png::image<png::rgb_pixel> cp_1("experiment/dum/1.png");
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile", "experiment/dum/2.png");
        png::image<png::rgb_pixel> cp_2("experiment/dum/2.png");
        npcr_2 = calculate_npcr(cp_1, cp_2);
      }
      std::cout << ip << " | ";
      std::cout << npcr_1.sum << " | ";
      std::cout << npcr_2.sum << " | ";
      std::cout << npcr_1.n;
      std::cout << '\n';
    }
  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
