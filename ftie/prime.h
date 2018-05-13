#ifndef PRIME_H
#define PRIME_H

#include <cstdint>
#include <vector>


class prime {
private:
  uint16_t N;
  bool * primes;
  void generate();

public:
  prime();
  prime(uint16_t maximumN);
  bool is_prime(uint16_t n);
  uint16_t next_prime(uint16_t n);

};

#endif
