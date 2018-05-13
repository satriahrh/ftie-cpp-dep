#include "bbs.h"

#include "prime.h"

#include <cstdint>
#include <vector>


bbs::bbs(){
  m = 21;
  x = 4;
}

bbs::bbs(uint16_t p, uint16_t q, uint64_t s) {
  uint16_t nPrime = p > q? p : q;
  prime prm(nPrime + 1);

  if (!prm.is_prime(p))
    throw "p is not prime";
  if (!prm.is_prime(q))
    throw "q is not prime";

  if (p % 4 != 3)
    throw "p != 3 (mod 4)";
  if (q % 4 != 3)
    throw "q != 3 (mod 4)";

  if (s == 0)
    throw "s = 0";

  m = p * q;

  if (s >= m)
    throw "s > m";

  if (std::gcd(m, s) != 1)
    throw "s is not co-prime to m";

  x = s;
}

uint8_t bbs::next() {
  x = (x * x) % m;
  return x;
}

std::vector<uint8_t> bbs::generate_keystream(uint_fast32_t n) {
  std::vector<uint8_t> keystream(n);
  for(uint_fast32_t i = 0; i < n; i++) {
    x = (x * x) % m;
    keystream[i] = x % UINT8_MAX;
  }
  return keystream;
}
