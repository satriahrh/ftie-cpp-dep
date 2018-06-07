#ifndef PRIME_H
#define PRIME_H

#include <cstdint>


class prime{
private:
  uint_fast16_t N;
  bool * primes;

public:
  prime(uint_fast16_t maximumN);
  ~prime();
  bool is_prime(uint_fast16_t n);
};

#endif
