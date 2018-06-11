#include "ftie/ftie.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <iostream>


int main(int argc, char* argv[]) {
  if (argc != 10) {
    if (argc == 8) {
      std::cout << "You are using depracated system" << std::endl;
    } else {
      std::cout << "Check given parameter is correct" << std::endl;
      return -1;
    }
  }
  const char* MODE = argv[1];
  const char* IN_FILE_PATH = argv[2];
  const char* OUT_FILE_PATH = argv[3];
  std::vector<uint8_t> KEYSTREAM;
  uint16_t P;
  uint16_t Q;
  uint32_t S;
  uint16_t A;
  uint16_t B;
  uint16_t N;
  if (argc == 10) {
    P = atoi(argv[4]);
    Q = atoi(argv[5]);
    S = atoi(argv[6]);
    A = atoi(argv[7]);
    B = atoi(argv[8]);
    N = atoi(argv[9]);
  } else if (argc == 8) {
    const char* KEYSTREAM_CHARS = argv[4];
    unsigned char* buffer = (unsigned char*)KEYSTREAM_CHARS;
    std::vector<uint8_t>::size_type size = std::strlen((const char*)buffer);
    std::vector<uint8_t> ks(buffer, buffer + size);
    KEYSTREAM = ks;
    A = atoi(argv[5]);
    B = atoi(argv[6]);
    N = atoi(argv[7]);
  }

  std::string encrypt("encrypt");
  std::string decrypt("decrypt");

  bool done = false;
  if (encrypt.compare(MODE) == 0) {
    try {
      std::cout << MODE << "ing " << IN_FILE_PATH << " to " << OUT_FILE_PATH << " . . . " << std::endl;
      auto start = std::chrono::high_resolution_clock::now();
      if (argc == 10)
        ftie::encrypt(P, Q, S, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
      else if (argc == 8)
        ftie::deprecated::encrypt(KEYSTREAM, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
      auto finish = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = finish - start;
      std::cout << "Done! Elapsed time: " << elapsed.count() << " seconds" << std::endl;
      done = true;
    } catch (const char * msg) {
      std::cout << "Oops, there seems to be a problem" << std::endl << "\tERROR: " << msg << std::endl;
    }
  } else if (decrypt.compare(MODE) == 0) {
    std::cout << MODE << "ing " << IN_FILE_PATH << " to " << OUT_FILE_PATH << " . . . " << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    try {
      if (argc == 10)
        ftie::decrypt(P, Q, S, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
      else if (argc == 8)
        ftie::deprecated::decrypt(KEYSTREAM, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
      auto finish = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = finish - start;
      std::cout << "Done! Elapsed time: " << elapsed.count() << " seconds"<< std::endl;
      done = true;
    } catch (const char * msg) {
      std::cout << "Oops, there seems to be a problem" << std::endl << "\tERROR: " << msg << std::endl;
    }
  }
  if (!done) {
    std::cout << "Oh no!! Something is wrong!" << std::endl;
  }

  return 0;
}
