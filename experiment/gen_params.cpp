// g++ -o bin/exp-gen-params -std=c++17 ftie/prime.h ftie/prime.cpp experiment/gen_params.cpp -lgmpxx -lgmp && ./bin/exp-gen-params
#include "../ftie/prime.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

#include <gmpxx.h>


int main() {
  const unsigned int STEPS = 30;
  const unsigned int MIN_BITS = 16;
  const unsigned int MAX_BITS = 1024;

  gmp_randclass rng(gmp_randinit_default);
  rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());

  std::ofstream out("experiment/bbs_params.csv", std::ios::out | std::ios::trunc);
  out << "bits,p,q,s\n";

  for (unsigned int i = 0; i < STEPS; i++) {
    unsigned int bits = MIN_BITS + static_cast<unsigned int>(
      static_cast<uint64_t>(i) * (MAX_BITS - MIN_BITS) / (STEPS - 1)
    );

    mpz_class p = ftie::generate_prime_congruent_3_mod_4(bits);
    mpz_class q;
    do {
      q = ftie::generate_prime_congruent_3_mod_4(bits);
    } while (q == p);

    mpz_class m = p * q;
    mpz_class s, g;
    do {
      s = rng.get_z_range(m - 2) + 2;
      mpz_gcd(g.get_mpz_t(), s.get_mpz_t(), m.get_mpz_t());
    } while (g != 1);

    out << bits << "," << p.get_str(10) << "," << q.get_str(10) << "," << s.get_str(10) << "\n";
    std::cout << "generated pair " << (i + 1) << "/" << STEPS << " (" << bits << " bits)" << std::endl;
  }

  out.close();
  return 0;
}
