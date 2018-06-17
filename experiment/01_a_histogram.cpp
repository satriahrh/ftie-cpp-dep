#include "../ftie/ftie.h"

#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>


// g++ -o bin/01-a -std=c++17 ftie/*.h ftie/*.cpp experiment/01_a_histogram.cpp `libpng-config --ldflags` && ./bin/01-a
int main(int argc, char const *argv[]) {
  try {
    std::random_device generateRandom;

    // SAME PLAINFILE, SAME KEYS
    {
      // plainfile
      std::vector<uint8_t> bytes(3210000);
      for (int i = 0; i < bytes.size(); i++)
      bytes[i] = generateRandom();
      std::ofstream outfile("experiment/dum/plainfile_hist", std::ios::out | std::ios::binary);
      outfile.write((const char*)&bytes[0], bytes.size());
      outfile.close();

      // keystream
      std::vector<uint8_t> keystream(bytes.size());
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

      for (int i = 1; i < 4; i++) {
        std::stringstream ss;
        ss << "experiment/dum/01-a/[s][s]_" << i << "_dep.png";
        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", ss.str().c_str());
        std::cout << "0" << i << " done" << '\n';
      }
      for (int i = 1; i < 4; i++) {
        std::stringstream ss;
        ss << "experiment/dum/01-a/[s][s]_" << i << ".png";
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", ss.str().c_str());\
        std::cout << "0" << i + 3 << " done" << '\n';
      }
    }

    // SAME PLAINFILE, VARIOUS KEYS
    {
      // plainfile
      std::random_device generateRandom;
      std::vector<uint8_t> bytes(3210000);
      for (int i = 0; i < bytes.size(); i++)
      bytes[i] = generateRandom();
      std::ofstream outfile("experiment/dum/plainfile_hist", std::ios::out | std::ios::binary);
      outfile.write((const char*)&bytes[0], bytes.size());
      outfile.close();

      {
        // keystream
        std::vector<uint8_t> keystream(bytes.size());
        for (int i = 0; i < keystream.size(); i++)
        keystream[i] = generateRandom();

        // bbs
        uint16_t p = 53051;
        uint16_t q = 59063;
        uint32_t s = 636977778;
        {
          // acm
          uint_fast16_t a = generateRandom();
          uint_fast16_t b = generateRandom();
          uint_fast16_t n = generateRandom() % 100;

          ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_discrete]_dep.png");
          std::cout << "07 done" << '\n';
          ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_discrete].png");
          std::cout << "08 done" << '\n';
        } {
          // acm
          uint_fast16_t a = generateRandom();
          uint_fast16_t n = generateRandom() % 100;

          ftie::deprecated::encrypt(keystream, a, a, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_equal]_dep.png");
          std::cout << "09 done" << '\n';
          ftie::encrypt(p, q, s, a, a, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_equal].png");
          std::cout << "10 done" << '\n';
        } {
          // acm
          uint_fast16_t a = 1;
          uint_fast16_t n = generateRandom() % 100;

          ftie::deprecated::encrypt(keystream, a, a, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_1]_dep.png");
          std::cout << "11 done" << '\n';
          ftie::encrypt(p, q, s, a, a, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][acm_1].png");
          std::cout << "12 done" << '\n';
        }
      } {
        // acm
        uint_fast16_t a = generateRandom();
        uint_fast16_t b = generateRandom();
        uint_fast16_t n = generateRandom() % 100;
        {
          // keystream
          std::vector<uint8_t> keystream(bytes.size());
          for (int i = 0; i < keystream.size(); i++)
          keystream[i] = generateRandom();

          // bbs
          uint16_t p = 17539;
          uint16_t q = 25667;
          uint32_t s = 252329327;

          ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_1]_dep.png");
          std::cout << "13 done" << '\n';
          ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_1].png");
          std::cout << "14 done" << '\n';
        } {
          // keystream
          std::vector<uint8_t> keystream(bytes.size() / 2);
          for (int i = 0; i < keystream.size(); i++)
          keystream[i] = generateRandom();

          // bbs
          uint16_t p = 55351;
          uint16_t q = 56843;
          uint32_t s = 879538636;

          ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_2]_dep.png");
          std::cout << "15 done" << '\n';
          ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_2].png");
          std::cout << "16 done" << '\n';
        } {
          // keystream
          std::vector<uint8_t> keystream(bytes.size() / 4);
          for (int i = 0; i < keystream.size(); i++)
          keystream[i] = generateRandom();

          // bbs
          uint16_t p = 61547;
          uint16_t q = 63611;
          uint32_t s = 2398929683;

          ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_4]_dep.png");
          std::cout << "17 done" << '\n';
          ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[s][keystream_4].png");
          std::cout << "18 done" << '\n';
        }
      }
    }

    // VARIOUS PLAINFILE, SAME KEYS
    {
      // keystream
      std::vector<uint8_t> keystream(3210000);
      for (int i = 0; i < keystream.size(); i++)
      keystream[i] = generateRandom();

      // bbs
      uint16_t p = 53699;
      uint16_t q = 54539;
      uint32_t s = 2814923085;

      // acm
      uint_fast16_t a = generateRandom();
      uint_fast16_t b = generateRandom();
      uint_fast16_t n = generateRandom() % 100;

      {
        // plainfile
        std::random_device generateRandom;
        std::vector<uint8_t> bytes(3210000);
        for (int i = 0; i < bytes.size(); i++)
        bytes[i] = generateRandom();
        std::ofstream outfile("experiment/dum/plainfile_hist", std::ios::out | std::ios::binary);
        outfile.write((const char*)&bytes[0], bytes.size());
        outfile.close();

        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_1_dep.png");
        std::cout << "19 done" << '\n';
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_1.png");
        std::cout << "20 done" << '\n';
      } {
        // plainfile
        std::random_device generateRandom;
        std::vector<uint8_t> bytes(3210000);
        for (int i = 0; i < bytes.size(); i++)
        bytes[i] = generateRandom();
        std::ofstream outfile("experiment/dum/plainfile_hist", std::ios::out | std::ios::binary);
        outfile.write((const char*)&bytes[0], bytes.size());
        outfile.close();

        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_2_dep.png");
        std::cout << "21 done" << '\n';
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_2.png");
        std::cout << "22 done" << '\n';
      } {
        // plainfile
        std::random_device generateRandom;
        std::vector<uint8_t> bytes(3210000);
        for (int i = 0; i < bytes.size(); i++)
        bytes[i] = generateRandom();
        std::ofstream outfile("experiment/dum/plainfile_hist", std::ios::out | std::ios::binary);
        outfile.write((const char*)&bytes[0], bytes.size());
        outfile.close();

        ftie::deprecated::encrypt(keystream, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_3_dep.png");
        std::cout << "23 done" << '\n';
        ftie::encrypt(p, q, s, a, b, n, "experiment/dum/plainfile_hist", "experiment/dum/01-a/[v][s]_3.png");
        std::cout << "24 done" << '\n';
      }
    }
  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
