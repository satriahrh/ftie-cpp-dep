#include "bbs.h"

#include "prime.h"

#include <cstdint>
#include <numeric>
#include <vector>


namespace ftie {
  namespace bbs {
    std::vector<uint8_t> generate_randoms(
      uint16_t p, uint16_t q, uint32_t s, uint_fast32_t n
    ) {
      {
        if (p < q) {
          uint16_t temp = q;
          q = p;
          p = temp;
        }
        prime prm(p + 1);
        if (!prm.is_prime(p) || !prm.is_prime(q))
          throw "p or q is not prime";
      }
      if (!(p % 4 == 3) || !(q % 4 == 3))
        throw "p or q is not congruence to 3 mod 4";

      uint64_t m = p * q;

      if (!(1 < s) || !(s < m))
        throw "s is not in (1, m)";
      if (std::gcd(s, m) != 1)
        throw "gcd(s,m) != 1";

      uint64_t x = s;
      std::vector<uint8_t> kbits_randoms(n);
      for(uint_fast32_t i = 0; i < n; i++) {
        x = x * x % m;\
        kbits_randoms[i] = x;
      }
      return kbits_randoms;
    }
  }
}
