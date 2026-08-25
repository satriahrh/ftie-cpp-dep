// g++ -o bin/exp-d -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags` && ./bin/exp-d
#include "../ftie/ftie.h"
#include "../experiment/tools.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <iostream>

#include "png++/png.hpp"


std::vector<uint16_t> P = {3, 1439, 3187, 5011, 6967, 9067, 11083, 13187, 15391, 17491, 19927, 22063, 24391, 26863, 29027, 31379, 33647, 35963, 38447, 40759, 43019, 45659, 48131, 50387, 52967, 55619, 57947, 60607, 62971, 65479};
std::vector<uint16_t> Q = {7, 1447, 3191, 5023, 6971, 9091, 11087, 13219, 15427, 17519, 19963, 22067, 24407, 26879, 29059, 31387, 33679, 35983, 38459, 40763, 43051, 45667, 48163, 50411, 52999, 55631, 57991, 60611, 62983, 65519};
std::vector<uint32_t> S = {20, 1102012, 8269800, 14654568, 8514112, 72359465, 8297696, 24662060, 111450600, 70082287, 235198879, 465814318, 180650199, 505938172, 200851637, 972337598, 426365137, 1242795649, 666777313, 704963584, 823914892, 993565933, 532403049, 2504300086, 1055897756, 1426192730, 3257141641, 359282555, 2439740097, 1549795366};

int main(int argc, char const *argv[]) {
  try {
    const char * slug = "acm_any_ab";
    std::stringstream ss;

    ss << "experiment/dum/" << slug << "_plainfile_1";
    const std::string plainfile1FilepathStr = ss.str();
    const char * plainfile1Filepath = plainfile1FilepathStr.c_str();
    ss.str("");

    ss << "experiment/dum/" << slug << "_plainfile_2";
    const std::string plainfile2FilepathStr = ss.str();
    const char * plainfile2Filepath = plainfile2FilepathStr.c_str();
    ss.str("");

    ss << "experiment/dum/" << slug << "_cipherimage_1";
    const std::string cipherimage1FilepathStr = ss.str();
    const char * cipherimage1Filepath = cipherimage1FilepathStr.c_str();
    ss.str("");

    ss << "experiment/dum/" << slug << "_cipherimage_2";
    const std::string cipherimage2FilepathStr = ss.str();
    const char * cipherimage2Filepath = cipherimage2FilepathStr.c_str();
    ss.str("");

    std::random_device generateRandom;

    // plainfile
    std::vector<uint8_t> bytes(3210000);
    for (int i = 0; i < bytes.size(); i++)
    bytes[i] = generateRandom();
    bytes[0] = 1;
    std::ofstream outfile1(plainfile1Filepath, std::ios::out | std::ios::binary);
    outfile1.write((const char*)&bytes[0], bytes.size());
    outfile1.close();

    bytes[0] = 3;
    std::ofstream outfile2(plainfile2Filepath, std::ios::out | std::ios::binary);
    outfile2.write((const char*)&bytes[0], bytes.size());
    outfile2.close();

    // keystream
    std::vector<uint8_t> keystream(10000000);
    for (int i = 0; i < keystream.size(); i++)
    keystream[i] = generateRandom();

    // bbs
    uint8_t bbs = generateRandom() % 30;
    uint16_t p = P[bbs];
    uint16_t q = Q[bbs];
    uint32_t s = S[bbs];

    // acm
    uint16_t n = 50;

    uint16_t init = 1;
    uint16_t inc = UINT16_MAX / 30;
    uint16_t max = UINT16_MAX - inc;
    for (uint16_t a = init; a <= max; a += inc) {
      uint16_t b = a + 1;

      double npcr_1;
      double npcr_2;
      double hm_1;
      double hm_2;
      double ava_1;
      double ava_2;
      std::chrono::duration<double> enc_1;
      std::chrono::duration<double> dec_1;
      std::chrono::duration<double> enc_2;
      std::chrono::duration<double> dec_2;

      {
        enc_1 = tools::chrono_deprecated_encrypt(keystream, a, b, n, plainfile1Filepath, cipherimage1Filepath);
        dec_1 = tools::chrono_deprecated_decrypt(keystream, a, b, n, cipherimage1Filepath, plainfile1Filepath);

        ftie::deprecated::encrypt(keystream, a, b, n, plainfile1Filepath, cipherimage2Filepath);
        npcr_1 = tools::calculate_npcr(cipherimage1Filepath, cipherimage2Filepath);
        hm_1 = tools::calculate_entropy(cipherimage1Filepath);

        ftie::deprecated::encrypt(keystream, a, b, n, plainfile2Filepath, cipherimage2Filepath);
        ava_1 = tools::calculate_avalanche(cipherimage1Filepath, cipherimage2Filepath);
      } {
        enc_2 = tools::chrono_encrypt(p, q, s, a, b, n, plainfile1Filepath, cipherimage1Filepath);
        dec_2 = tools::chrono_decrypt(p, q, s, a, b, n, cipherimage1Filepath, plainfile1Filepath);

        ftie::encrypt(p, q, s, a, b, n, plainfile1Filepath, cipherimage2Filepath);
        npcr_2 = tools::calculate_npcr(cipherimage1Filepath, cipherimage2Filepath);
        hm_2 = tools::calculate_entropy(cipherimage1Filepath);

        ftie::encrypt(p, q, s, a, b, n, plainfile2Filepath, cipherimage2Filepath);
        ava_2 = tools::calculate_avalanche(cipherimage1Filepath, cipherimage2Filepath);
      }
      std::cout << a << ", ";

      std::cout << npcr_1 << ", ";
      std::cout << hm_1 << ", ";
      std::cout << ava_1 << ", ";
      std::cout << enc_1.count() << ", ";
      std::cout << dec_1.count() << ", ";

      std::cout << npcr_2 << ", ";
      std::cout << hm_2 << ", ";
      std::cout << ava_2 << ", ";
      std::cout << enc_2.count() << ", ";
      std::cout << dec_2.count() << "\n";
    }
  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
