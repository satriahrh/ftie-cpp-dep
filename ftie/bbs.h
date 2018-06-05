#ifndef BBS_H
#define BBS_H

#include <cstdint>
#include <vector>


namespace ftie {
  namespace bbs {
    std::vector<uint8_t> generate_keystream(
      uint16_t p, uint16_t q, uint32_t s, uint_fast32_t n
    );
  }
}

#endif
