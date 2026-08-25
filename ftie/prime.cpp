#include "prime.h"

#include <chrono>

#include <gmpxx.h>


namespace ftie {
  bool is_prime(const mpz_class& n) {
    return mpz_probab_prime_p(n.get_mpz_t(), MILLER_RABIN_ROUNDS) != 0;
  }

  mpz_class generate_prime_congruent_3_mod_4(unsigned int bits) {
    gmp_randclass rng(gmp_randinit_default);
    rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    mpz_class candidate;
    do {
      candidate = rng.get_z_bits(bits);
      mpz_setbit(candidate.get_mpz_t(), bits - 1);
      candidate |= 3;
    } while (!is_prime(candidate));
    return candidate;
  }
}
