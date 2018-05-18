#include "prime.h"

#include <cstdint>
#include <fstream>


prime::prime(const char * primeFilePath) {
  uint16_t buffer;
  std::ifstream primeFile(primeFilePath, std::ios::binary);
  while (primeFile.read((char*)&buffer, 2))
    primes.insert(buffer);
  primeFile.close();
}

bool prime::is_prime(uint16_t n) {
  bool verdict = primes.find(n) != primes.end();
  return verdict;
}
