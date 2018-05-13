#include "prime.h"

#include <cmath>    // std::fabs
#include <cstdint>

uint16_t iroot(uint16_t n) {
  // https://en.wikipedia.org/wiki/Integer_square_root#Algorithm_using_Newton's_method
  float xk = n * 1.0;
  float xkp1 = (xk + (n / xk)) / 2.0;
  while (std::fabs(xkp1 - xk) >= 1) {
    xk = xkp1;
    xkp1 = (xk + (n / xk)) / 2.0;
  }
  return uint16_t(xkp1);
}

void prime::generate() {
  primes = new bool[N];
  uint16_t limit = iroot(N);
  for(uint16_t i = 2; i <= limit; i++)
    if (!primes[i])
      for(uint16_t j = i + i; j < N; j += i)
        primes[j] = true;
}

bool prime::is_prime(uint16_t n) {
  return n >= N ? false : !primes[n];
}

uint16_t prime::next_prime(uint16_t n) {
  for (uint16_t i = n + 1; i < N; i++) {
    if (primes[i])
      return i;
  }
  return 0;
}
