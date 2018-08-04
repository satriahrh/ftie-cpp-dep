#ifndef TOOLS_H
#define TOOLS_H

#include <chrono>
#include <cstdint>
#include <vector>

namespace tools {
  double calculate_avalanche(
    const char * path_1,
    const char * path_2
  );
  double calculate_entropy(
    const char * cipherimageFilepath
  );
  double calculate_npcr(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  );
  std::chrono::duration<double> chrono_encrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_decrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );

  std::chrono::duration<double> chrono_deprecated_encrypt(
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_deprecated_decrypt(
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
}

#endif
