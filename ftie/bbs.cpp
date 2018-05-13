#include "bbs.h"

#include <cstdint>
#include <vector>

uint8_t bbs::next() {
  x = (x * x) % m;
  return x;
}

std::vector<uint8_t> bbs::generate_keystream(uint_fast32_t n) {
  std::vector<uint8_t> keystream(n);
  for(uint_fast32_t i = 0; i < n; i++) {
    x = (x * x) % m;
    keystream[i] = x % UINT8_MAX;
  }
  return keystream;
}
