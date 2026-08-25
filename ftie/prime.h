#ifndef PRIME_H
#define PRIME_H

#include <gmpxx.h>


namespace ftie {
  // Miller-Rabin round count; mpz_probab_prime_p's false-positive bound is
  // 4^-MILLER_RABIN_ROUNDS (<= 2^-80 at 40 rounds).
  constexpr int MILLER_RABIN_ROUNDS = 40;

  bool is_prime(const mpz_class& n);
  mpz_class generate_prime_congruent_3_mod_4(unsigned int bits);
}

#endif
