#include "suplementary.h"

#include <cmath>

uint32_t iroot(uint64_t n) {
  // https://en.wikipedia.org/wiki/Integer_square_root#Algorithm_using_Newton's_method
  long double xk = n * 1.0;
  long double xkp1 = (xk + (n / xk)) / 2.0;
  while (std::fabs(xkp1 - xk) >= 1) {
    xk = xkp1;
    xkp1 = (xk + (n / xk)) / 2.0;
  }
  return uint32_t(xkp1);
}
