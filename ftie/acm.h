#ifndef ACM_H
#define ACM_H

#include <cstdint>
#include <vector>

class acm {
private:
  uint_fast16_t a;
  uint_fast16_t b;
  uint_fast16_t n;
  bool type;
  std::vector<std::vector<std::vector<uint_fast16_t>>> get_map(uint_fast16_t N);
  std::vector<std::vector<std::vector<uint_fast16_t>>> mapping_equal(uint_fast16_t N);
  std::vector<std::vector<std::vector<uint_fast16_t>>> mapping_general(uint_fast16_t N);

public:
  acm(uint_fast16_t acmA, uint_fast16_t acmB, uint_fast16_t acmN) {
    a = acmA;
    b = acmB;
    n = acmN;
    if (a == b) type = 0;
    else type = 1;
  };
  std::vector<std::vector<std::vector<uint8_t>>> encrypt(std::vector<std::vector<std::vector<uint8_t>>> plainmatrix);
  std::vector<std::vector<std::vector<uint8_t>>> decrypt(std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix);
};

#endif
