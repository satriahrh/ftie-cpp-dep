#include "old_acm.h"

#include <cstdint>
#include <cmath>
#include <vector>

uint_fast16_t iroot(uint_fast32_t n) {
  // https://en.wikipedia.org/wiki/Integer_square_root#Algorithm_using_Newton's_method
  double xk = n * 1.0;
  double xkp1 = (xk + (n / xk)) / 2.0;
  while (std::fabs(xkp1 - xk) >= 1) {
    xk = xkp1;
    xkp1 = (xk + (n / xk)) / 2.0;
  }
  return uint_fast16_t(xkp1);
}

std::vector<uint_fast32_t> old_acm::get_map(uint_fast16_t a, uint_fast16_t b, uint_fast32_t l) {
  uint_fast16_t n = iroot(l) + 1;
  std::vector<uint_fast32_t> s;
  for(uint_fast16_t x = 0; x < n; x++) {
    for(uint_fast16_t y = 0; y < n; y++) {
      uint_fast16_t mx = (x + (a * y % n)) % n;
      uint_fast16_t my = ((b * x % n) + (((a * b % n)+ 1) * y % n)) % n;
      uint_fast32_t ms = n * mx + my;
      if (ms < l)
        s.push_back(ms);
    }
  }
  return s;
}

std::vector<uint8_t> old_acm::encrypt(uint_fast16_t a, uint_fast16_t b, std::vector<uint8_t> plainbytes) {
  uint_fast32_t l = plainbytes.size();
  std::vector<uint_fast32_t> map = get_map(a, b, l);
  std::vector<uint8_t> cipherbytes(l);
  for(uint_fast32_t i = 0; i < l; i++) {
    cipherbytes[i] = plainbytes[map[i]];
  }
  return cipherbytes;
}

std::vector<uint8_t> old_acm::decrypt(uint_fast16_t a, uint_fast16_t b, std::vector<uint8_t> cipherbytes) {
  uint_fast32_t l = cipherbytes.size();
  std::vector<uint_fast32_t> map = get_map(a, b, l);
  std::vector<uint8_t> plainbytes(l);
  for(uint_fast32_t i = 0; i < l; i++) {
    plainbytes[map[i]] = cipherbytes[i];
  }
  return plainbytes;
}
