#include "ftie/ftie.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <iostream>


int main(int argc, char* argv[]) {
  if (argc != 10) {
    std::cout << "Check given parameter is correct" << std::endl;
    return -1;
  }
  const char* MODE = argv[1];
  const char* IN_FILE_PATH = argv[2];
  const char* OUT_FILE_PATH = argv[3];
  uint16_t P = atoi(argv[4]);
  uint16_t Q = atoi(argv[5]);
  uint64_t S = atoi(argv[6]);
  uint_fast16_t A = atoi(argv[7]);
  uint_fast16_t B = atoi(argv[8]);
  uint_fast16_t N = atoi(argv[9]);


  std::string encrypt("encrypt");
  std::string decrypt("decrypt");

  ftie ftieApp;
  bool done = false;
  if (encrypt.compare(MODE) == 0) {
    std::cout << MODE << "ing " << IN_FILE_PATH << " to " << OUT_FILE_PATH << " . . . " << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    ftieApp.encrypt(P, Q, S, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Done! Elapsed time: " << elapsed.count() << " seconds" << std::endl;
    done = true;
  } else if (decrypt.compare(MODE) == 0) {
    std::cout << MODE << "ing " << IN_FILE_PATH << " to " << OUT_FILE_PATH << " . . . " << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    ftieApp.decrypt(P, Q, S, A, B, N, IN_FILE_PATH, OUT_FILE_PATH);
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Done! Elapsed time: " << elapsed.count() << " seconds"<< std::endl;
    done = true;
  }
  if (!done) {
    std::cout << "Oh no!! Something is wrong!" << std::endl;
  }

  return 0;
}
