#include "../ftie/ftie.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>


// g++ -o bin/03-a -std=c++17 ftie/*.h ftie/*.cpp experiment/03_a_time.cpp `libpng-config --ldflags` && ./bin/03-a
int main(int argc, char const *argv[]) {
  try {
    uint16_t inc = 10;
    uint16_t max = 300;
    uint16_t init = 10;

    const char * title = "ACM Equal n";
    const char * slug = "acm_equal_n";

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

    // bbs
    uint16_t p = 55603;
    uint16_t q = 57467;
    uint32_t s = 1369318511;

    uint16_t a = generateRandom();

    std::cout << "## " << title << '\n';

    std::cout << "### Encryption\nn | deprecated (s) | proposed (s)\n--- | --- | ---\n";
    for (uint16_t n = init; n <= max; n += inc) {
      std::cout << n << " | ";
      {
        ss << "experiment/dum/time_cipherimage_" << slug << "_dep_" << n << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::deprecated::encrypt(keystream, a, a, n, plainfileFilepath, ss.str().c_str());
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << " | ";
        ss.str("");
      } {
        ss << "experiment/dum/time_cipherimage_" << slug << "_pro_" << n << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::encrypt(p, q, s, a, a, n, plainfileFilepath, ss.str().c_str());
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << '\n';
        ss.str("");
      }
    } std::cout << '\n';

    std::cout << "### Decryption\nn | deprecated (s) | proposed (s)\n--- | --- | ---\n";
    for (uint16_t n = init; n <= max; n += inc) {
      std::cout << n << " | ";
      {
        ss << "experiment/dum/time_cipherimage_" << slug << "_dep_" << n << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::deprecated::decrypt(keystream, a, a, n, ss.str().c_str(), plainfileFilepath);
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << elapsed.count() << " | ";
        ss.str("");
      } {
        ss << "experiment/dum/time_cipherimage_" << slug << "_pro_" << n << ".png";
        auto start = std::chrono::high_resolution_clock::now();
        ftie::decrypt(p, q, s, a, a, n, ss.str().c_str(), plainfileFilepath);
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
