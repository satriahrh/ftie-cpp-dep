#include "../ftie/ftie.h"

#include <cstdint>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <iostream>


double calculate_entropy(
  png::image<png::rgb_pixel> image
) {
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

// g++ -o bin/02 -std=c++17 ftie/*.h ftie/*.cpp experiment/02_entropy.cpp `libpng-config --ldflags` && ./bin/02
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
    uint_fast16_t n = generateRandom() % 300;

    // bbs
    uint16_t p = 55603;
    uint16_t q = 57467;
    uint32_t s = 1369318511;

    std::cout << "# Entropy" << '\n';
    std::cout << "plainfile's size (MB) | deprecated | proposed" << '\n';
    std::cout << "--- | --- | ---" << '\n';

    uint16_t init = 5;
    uint16_t inc = 5;
    uint16_t max = 100;
    for (uint16_t mb = init; mb <= max; mb += inc) {
      // plainfile
      std::vector<uint8_t> bytes(mb * 100000);
      for (int i = 0; i < bytes.size(); i++)
      bytes[i] = generateRandom();
      std::ofstream outfile("experiment/dum/plainfile_ent", std::ios::out | std::ios::binary);
      outfile.write((const char*)&bytes[0], bytes.size());
      outfile.close();

      double hm_1;
      double hm_2;
      {
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_ent", "experiment/dum/1_ent.png");
        png::image<png::rgb_pixel> cipherimage("experiment/dum/1_ent.png");
        hm_1 = calculate_entropy(cipherimage);
      } {
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_ent", "experiment/dum/1_ent.png");
        png::image<png::rgb_pixel> cipherimage("experiment/dum/1_ent.png");
        hm_2 = calculate_entropy(cipherimage);
      }
      std::cout << (mb / 10.0) << " | ";
      std::cout << hm_1 << " | ";
      std::cout << hm_2;
      std::cout << '\n';
    }
  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
