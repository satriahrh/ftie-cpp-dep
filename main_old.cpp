#include "ftie/old_ftie.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <iostream>


int main(int argc, char* argv[]) {
  if (argc != 7) {
    std::cout << "Check given parameter is correct" << std::endl;
    return -1;
  }
  const char* MODE = argv[1];
  const char* IN_FILE_PATH = argv[2];
  const char* OUT_FILE_PATH = argv[3];
  const char* KEYSTRAEM_FILE_PATH = argv[4];
  uint_fast16_t A = atoi(argv[5]);
  uint_fast16_t B = atoi(argv[6]);

  // READING KEYSTREAM
  std::vector<uint8_t> keystream;
  std::string line;
  std::ifstream keystreamFile(KEYSTRAEM_FILE_PATH);
  while (getline(keystreamFile, line)) {
    keystream.push_back(stoi(line));
  }

  keystreamFile.close();

  std::string encrypt("encrypt");
  std::string decrypt("decrypt");

  old_ftie ftieApp;
  bool done = false;
  if (encrypt.compare(MODE) == 0) {
    try {
      std::cout << MODE << "ing " << IN_FILE_PATH << " to " << OUT_FILE_PATH << " . . . " << std::endl;
      auto start = std::chrono::high_resolution_clock::now();
      ftieApp.encrypt(keystream, A, B, IN_FILE_PATH, OUT_FILE_PATH);
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
      ftieApp.decrypt(keystream, A, B, IN_FILE_PATH, OUT_FILE_PATH);
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
