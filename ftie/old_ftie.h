#ifndef OLD_FTIE_H
#define OLD_FTIE_H

#include "ftie.h"
#include "old_acm.h"
#include "rt.h"

#include <vector>

class old_ftie: public ftie {
public:
  void encrypt(
    std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );
  void decrypt(
    std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
};

#endif
