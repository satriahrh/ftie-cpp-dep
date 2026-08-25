#include "bbs.h"

#include "prime.h"

#include <cstdint>
#include <vector>

#include <gmpxx.h>


namespace ftie {
  namespace bbs {
    std::vector<uint8_t> generate_randoms(
      mpz_class p, mpz_class q, mpz_class s, uint32_t n
    ) {
      {
        if (p < q) {
          mpz_class temp = q;
          q = p;
          p = temp;
        }
        if (!ftie::is_prime(p) || !ftie::is_prime(q))
          throw "p or q is not prime";
      }
      if (p % 4 != 3 || q % 4 != 3)
        throw "p or q is not congruence to 3 mod 4";

      mpz_class m = p * q;

      if (!(s > 1) || !(s < m))
        throw "s is not in (1, m)";
      mpz_class g;
      mpz_gcd(g.get_mpz_t(), s.get_mpz_t(), m.get_mpz_t());
      if (g != 1)
        throw "gcd(s,m) != 1";

      mpz_class x = s;
      std::vector<uint8_t> kbits_randoms(n);
      for(uint32_t i = 0; i < n; i++) {
        x = (x * x) % m;
        mpz_class low_byte = x & mpz_class(0xFF);
        kbits_randoms[i] = static_cast<uint8_t>(low_byte.get_ui());
      }
      return kbits_randoms;
    }
  }
}
