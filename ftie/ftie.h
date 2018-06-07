#ifndef FTIE_H
#define FTIE_H

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <vector>

namespace ftie {
  void encrypt(
    uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );
  void decrypt(
    uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
}

#endif
