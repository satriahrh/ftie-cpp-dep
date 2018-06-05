#ifndef ACM_H
#define ACM_H

#include "png++/png.hpp"

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
  acm(uint_fast16_t acmA, uint_fast16_t acmB, uint_fast16_t acmN);
  png::image<png::rgb_pixel> encrypt(png::image<png::rgb_pixel> plainimage);
  png::image<png::rgb_pixel> decrypt(png::image<png::rgb_pixel> cipherimage);
};

#endif
