#ifndef PRIME_H
#define PRIME_H

#include <cstdint>


class prime{
private:
  uint32_t N;
  bool * primes;

  void generate();

public:
  prime();
  prime(uint32_t maximumN);
  bool is_prime(uint32_t n);
  uint32_t next_prime(uint32_t n);
};

#endif
