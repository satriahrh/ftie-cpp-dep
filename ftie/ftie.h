#ifndef FTIE_H
#define FTIE_H

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <gmpxx.h>

#include <vector>


namespace ftie {
  // Returns the seed for the next message under the same p, q (the last BBS
  // state); decrypt must be given the seed s that this message was encrypted with.
  mpz_class encrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );
  void decrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );

  namespace previous {
    void encrypt(
      std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
      const char* plainfileFilepath, const char* cipherimageFilepath
    );
    void decrypt(
      std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
      const char* cipherimageFilepath, const char* plainfileFilepath
    );
  }
}

#endif
