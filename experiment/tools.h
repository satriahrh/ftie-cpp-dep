#ifndef TOOLS_H
#define TOOLS_H

#include <chrono>
#include <cstdint>
#include <vector>

#include <gmpxx.h>

namespace tools {
  struct BbsParams {
    unsigned int bits;
    mpz_class p, q, s;
  };

  std::vector<BbsParams> read_bbs_params(const char* csvFilepath);

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
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_decrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
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
