#ifndef OLD_FTIE_H
#define OLD_FTIE_H

#include "ftie.h"

#include <vector>

class old_ftie: public ftie {
  void encrypt(
    std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );
  void decrypt(
    std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
};

#endif
