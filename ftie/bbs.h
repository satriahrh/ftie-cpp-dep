#ifndef BBS_H
#define BBS_H

#include <cstdint>
#include <vector>

#include <gmpxx.h>


namespace ftie {
  namespace bbs {
    std::vector<uint8_t> generate_randoms(
      mpz_class p, mpz_class q, mpz_class s, uint32_t n
    );
  }
}

#endif
