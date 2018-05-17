#include "prime.h"

#include "suplementary.h"

#include <cstdint>


prime::prime() {
  N = UINT32_MAX;
  generate();
}

prime::prime(uint32_t maximumN) {
  N = maximumN;
  generate();
}

prime::~prime() {
  delete [] primes;
}

void prime::generate() {
  primes = new bool[N];
  uint16_t limit = iroot(N);
  for(uint16_t i = 2; i <= limit; i++)
    if (!primes[i])
      for(uint16_t j = i + i; j < N; j += i)
        primes[j] = true;
}

bool prime::is_prime(uint32_t n) {
  return n >= N ? false : !primes[n];
}

uint32_t prime::next_prime(uint32_t n) {
  for (uint32_t i = n + 1; i < N; i++) {
    if (!primes[i])
      return i;
  }
  return 0;
}
