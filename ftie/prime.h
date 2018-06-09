#ifndef PRIME_H
#define PRIME_H

#include <cstdint>


class prime{
private:
  uint16_t N;
  bool * primes;

public:
  prime(uint16_t maximumN);
  ~prime();
  bool is_prime(uint16_t n);
};

#endif
