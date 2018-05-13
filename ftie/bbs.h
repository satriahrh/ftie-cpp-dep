#ifndef BBS_H
#define BBS_H

#include "prime.h"

#include <cstdint>
#include <numeric>
#include <vector>


class bbs {
private:
  uint64_t m;
  uint64_t x;

public:
  bbs();
  bbs(uint16_t p, uint16_t q, uint64_t s);
  uint8_t next();

  std::vector<uint8_t> generate_keystream(uint_fast32_t n);
};

#endif
