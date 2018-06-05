#ifndef ACM_H
#define ACM_H

#include "png++/png.hpp"

#include <cstdint>
#include <vector>


namespace ftie {
  namespace acm {
    png::image<png::rgb_pixel> encrypt(
      uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
      png::image<png::rgb_pixel> plainimage
    );
    png::image<png::rgb_pixel> decrypt(
      uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
      png::image<png::rgb_pixel> cipherimage
    );
  }
}

#endif
