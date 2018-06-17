#include "../ftie/ftie.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>


// g++ -o bin/03-e -std=c++17 ftie/*.h ftie/*.cpp experiment/03_e_time.cpp `libpng-config --ldflags` && ./bin/03-e
int main(int argc, char const *argv[]) {
  try {
    uint16_t init = 0;
    uint16_t inc = 1;
    uint16_t max = 29;

    const char * title = "RT p q";
    const char * slug = "rt_p_q";

    std::random_device generateRandom;
    std::stringstream ss;

    ss << "experiment/dum/time_plainfile_" << slug << "";
    const std::string plainfileFilepathStr = ss.str();
    const char * plainfileFilepath = plainfileFilepathStr.c_str();
    ss.str("");

    // plainfile
    std::vector<uint8_t> bytes(3210000);
    for (int i = 0; i < bytes.size(); i++)
    bytes[i] = generateRandom();
    std::ofstream outfile(plainfileFilepath, std::ios::out | std::ios::binary);
    outfile.write((const char*)&bytes[0], bytes.size());
    outfile.close();

    // keystream
    std::vector<uint8_t> keystream(bytes.size());
    for (int i = 0; i < keystream.size(); i++)
    keystream[i] = generateRandom();

    // acm
    uint16_t a = generateRandom();
    uint16_t b = generateRandom();
    uint16_t n = generateRandom() % 300;

    std::vector<uint16_t> P = {3, 1439, 3187, 5011, 6967, 9067, 11083, 13187, 15391, 17491, 19927, 22063, 24391, 26863, 29027, 31379, 33647, 35963, 38447, 40759, 43019, 45659, 48131, 50387, 52967, 55619, 57947, 60607, 62971, 65479};
    std::vector<uint16_t> Q = {7, 1447, 3191, 5023, 6971, 9091, 11087, 13219, 15427, 17519, 19963, 22067, 24407, 26879, 29059, 31387, 33679, 35983, 38459, 40763, 43051, 45667, 48163, 50411, 52999, 55631, 57991, 60611, 62983, 65519};
    std::vector<uint32_t> S = {20, 1102012, 8269800, 14654568, 8514112, 72359465, 8297696, 24662060, 111450600, 70082287, 235198879, 465814318, 180650199, 505938172, 200851637, 972337598, 426365137, 1242795649, 666777313, 704963584, 823914892, 993565933, 532403049, 2504300086, 1055897756, 1426192730, 3257141641, 359282555, 2439740097, 1549795366};

    std::cout << "## " << title << '\n';

    std::cout << "### Encryption\np | q | deprecated (s) | proposed (s)\n--- | --- | --- | ---\n";
    for (uint16_t i = init; i <= max; i += inc) {
      uint16_t p = P[i];
      uint16_t q = Q[i];
      uint32_t s = S[i];
      std::cout << p << " | " << q << " | ";
      {
        ss << "experiment/dum/time_cipherimage_" << slug << "_dep_" << p << '_' << q << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::deprecated::encrypt(keystream, a, b, n, plainfileFilepath, ss.str().c_str());
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << " | ";
        ss.str("");
      } {
        ss << "experiment/dum/time_cipherimage_" << slug << "_pro_" << p << '_' << q << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::encrypt(p, q, s, a, b, n, plainfileFilepath, ss.str().c_str());
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << '\n';
        ss.str("");
      }
    } std::cout << '\n';

    std::cout << "### Decryption\np | q | deprecated (s) | proposed (s)\n--- | --- | --- | ---\n";
    for (uint16_t i = init; i <= max; i += inc) {
      uint16_t p = P[i];
      uint16_t q = Q[i];
      uint32_t s = S[i];
      std::cout << p << " | " << q << " | ";
      {
        ss << "experiment/dum/time_cipherimage_" << slug << "_dep_" << p << '_' << q << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::deprecated::decrypt(keystream, a, b, n, ss.str().c_str(), plainfileFilepath);
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << " | ";
        ss.str("");
      } {
        ss << "experiment/dum/time_cipherimage_" << slug << "_pro_" << p << '_' << q << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::decrypt(p, q, s, a, b, n, ss.str().c_str(), plainfileFilepath);
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << '\n';
        ss.str("");
      }
    } std::cout << '\n';

  } catch (const char * msg) {
    std::cout << msg << '\n';
  }
  return 0;
}
