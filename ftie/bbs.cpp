#include "bbs.h"

#include "prime.h"

#include <cstdint>
#include <vector>


bbs::bbs(){
  m = 21;
  x = 4;
}

bbs::bbs(uint16_t p, uint16_t q, uint32_t s) {
  uint16_t nPrime = p > q? p : q;
  prime prm;

  if (!prm.is_prime(p))
    throw "p is not prime or p != 3 (mod 4)";
  if (!prm.is_prime(q))
    throw "q is not prime or p != 3 (mod 4)";

  if (s == 0)
    throw "s = 0";

  m = p * q;

  if (s >= m)
    throw "s > m";

  if (std::gcd(m, s) != 1)
    throw "s is not co-prime to m";

  x = s;
}

std::vector<uint8_t> bbs::generate_keystream(uint_fast32_t n) {
  std::vector<uint8_t> keystream(n);
  for(uint_fast32_t i = 0; i < n; i++) {
    x = x * x % m;
    keystream[i] = x;
  }
  return keystream;
}
