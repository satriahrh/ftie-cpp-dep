#ifndef BBS_H
#define BBS_H

#include <cstdint>
#include <vector>

#include <gmpxx.h>


namespace ftie {
  namespace bbs {
    // K_i = x_i mod 256 for i = 1..n, with x_0 = s and x_i = x_{i-1}^2 mod pq.
    // next_seed receives x_n, the seed of the next message under the same key.
    std::vector<uint8_t> generate_randoms(
      mpz_class p, mpz_class q, mpz_class s, uint32_t n, mpz_class & next_seed
    );
  }
}

#endif
