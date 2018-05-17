#ifndef PRIME_H
#define PRIME_H

#include <cstdint>
#include <unordered_set>


class prime{
private:
  std::unordered_set<uint16_t> primes;

public:
  prime();
  bool is_prime(uint16_t n);
};

#endif
