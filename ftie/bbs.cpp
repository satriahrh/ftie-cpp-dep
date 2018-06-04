#include "bbs.h"

#include <cstdint>
#include <vector>


bbs::bbs(){
  m = 21;
  x = 4;
}

bbs::bbs(uint16_t p, uint16_t q, uint32_t s) {
  m = p * q;
  x = s;
}

std::vector<uint8_t> bbs::generate_keystream(uint_fast32_t n) {
  std::vector<uint8_t> keystream(n);
  for(uint_fast32_t i = 0; i < n; i++) {
    x = x * x % m;
    keystream[i] = x;
  }
  return keystream;
}
