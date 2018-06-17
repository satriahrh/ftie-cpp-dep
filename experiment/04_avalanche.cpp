#include "../ftie/ftie.h"

#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <iostream>


double calculate_avalanche(
  const char * path_1,
  const char * path_2
) {
  std::ifstream fst(path_1, std::ios::in | std::ios::binary);
  std::ifstream snd(path_2, std::ios::in | std::ios::binary);
  uint64_t n = 0;
  uint64_t mutual = 0;
  char cfst;
  char csnd;
  int count = 0;
  while (fst.get(cfst) && snd.get(csnd)) {
    for (int i = 7; i >= 0; i--) {
      bool a = ((cfst >> i) & 1);
      bool b = ((csnd >> i) & 1);
      if (a == b)
        mutual++;
      n++;
    }
  }
  double n_double = n;
  return mutual / n_double;
}

// g++ -o bin/04 -std=c++17 ftie/*.h ftie/*.cpp experiment/04_avalanche.cpp `libpng-config --ldflags` && ./bin/04
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

    std::cout << "# Avalanche Effect" << '\n';
    std::cout << "plainfile's size (MB) | deprecated | proposed" << '\n';
    std::cout << "--- | --- | ---" << '\n';

    uint16_t init = 5;
    uint16_t inc = 5;
    uint16_t max = 100;
    for (uint16_t mb = init; mb <= max; mb += inc) {
      // plainfile_avl
      std::vector<uint8_t> bytes_1(mb * 100000);
      std::vector<uint8_t> bytes_2(mb * 100000);
      for (int i = 0; i < bytes_1.size(); i++) {
        uint8_t bt = generateRandom();
        if (i == 0) {
          bytes_1[i] = 1;
          bytes_2[i] = 0;
        } else {
          bytes_1[i] = generateRandom();
          bytes_2[i] = generateRandom();
        }
      }
      std::ofstream outfile_1("experiment/dum/plainfile_avl_1", std::ios::out | std::ios::binary);
      outfile_1.write((const char*)&bytes_1[0], bytes_1.size());
      outfile_1.close();

      std::ofstream outfile_2("experiment/dum/plainfile_avl_2", std::ios::out | std::ios::binary);
      outfile_2.write((const char*)&bytes_2[0], bytes_2.size());
      outfile_2.close();

      double avalanche_1;
      double avalanche_2;
      {
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_avl_1", "experiment/dum/1_avl.png");
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_avl_2", "experiment/dum/2_avl.png");
        avalanche_1 = calculate_avalanche("experiment/dum/1_avl.png", "experiment/dum/2_avl.png");
      } {
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_avl_1", "experiment/dum/1_avl.png");
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_avl_2", "experiment/dum/2_avl.png");
        avalanche_2 = calculate_avalanche("experiment/dum/1_avl.png", "experiment/dum/2_avl.png");
      }
      std::cout << (mb / 10.0) << " | ";
      std::cout << avalanche_1 << " | ";
      std::cout << avalanche_2 << '\n';
    }
  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
