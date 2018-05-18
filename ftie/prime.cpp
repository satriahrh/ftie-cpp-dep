#include "prime.h"

#include <cstdint>
#include <fstream>
#include <iterator>


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

uint64_t prime::next(uint16_t p) { // p should be in primes
  std::set<uint16_t>::iterator it = primes.find(p);
  if (it == primes.end())
    throw "p is not primes in binary prime";

  it++;

  return it != primes.end()? *it : 0;
}

std::vector<uint16_t> prime::to_vector () {
  std::vector<uint16_t> vec(primes.size());
  std::copy(primes.begin(), primes.end(), vec.begin());

  return vec;
};
