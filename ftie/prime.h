#ifndef PRIME_H
#define PRIME_H

#include <cstdint>
#include <set>
#include <vector>


class prime{
private:
  std::set<uint16_t> primes;

public:
  prime(const char * primeFilePath = "./data/primes_2_3mod4.bin");
  bool is_prime(uint16_t n);
  uint64_t next(uint16_t p); // p should be in primes
  std::vector<uint16_t> to_vector();
};

#endif
