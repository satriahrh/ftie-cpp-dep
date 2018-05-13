#ifndef BBS_H
#define BBS_H

#include <cstdint>
#include <vector>

class prime {
private:
  uint16_t N;
  bool * primes;
  void generate();

public:
  prime() {
    N = UINT16_MAX;
    generate();
  }
  prime(uint16_t maximumN) {
    if (maximumN > UINT16_MAX)
      throw "maximumN exceeds UINT16_MAX";
    N = maximumN;
    generate();
  }
  bool is_prime(uint16_t n);
  uint16_t next_prime(uint16_t n);

};

#endif
