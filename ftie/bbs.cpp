#include "bbs.h"

#include <cstdint>
#include <vector>


namespace ftie {
  namespace bbs {
    std::vector<uint8_t> generate_keystream(
      uint16_t p, uint16_t q, uint32_t s, uint_fast32_t n
    ) {
      uint64_t m = p * q;
      uint64_t x = s;
      std::vector<uint8_t> keystream(n);
      for(uint_fast32_t i = 0; i < n; i++) {
        x = x * x % m;\
        keystream[i] = x;
      }
      return keystream;
    }
  }
}
