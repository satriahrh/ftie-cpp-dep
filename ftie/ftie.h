#ifndef FTIE_H
#define FTIE_H

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <vector>


class ftie {
public:
  std::vector<uint8_t> read_plainfile(const char* filePath);
  std::vector<uint8_t> pad_bytes(std::vector<uint8_t> plainbytes);
  std::vector<std::vector<std::vector<uint8_t>>> bytes_to_matrix(std::vector<uint8_t> cipherbytes);
  png::image<png::rgb_pixel> matrix_to_image(std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix);

  std::vector<std::vector<std::vector<uint8_t>>> image_to_matrix(png::image<png::rgb_pixel> cipherimage);
  std::vector<uint8_t> matrix_to_bytes(std::vector<std::vector<std::vector<uint8_t>>> plainmatrix);
  std::vector<uint8_t> strip_bytes(std::vector<uint8_t> plainbytes);
  void write_plainfile(std::vector<uint8_t>, const char* filePath);

  void encrypt(
    uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );
  void decrypt(
    uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
};

#endif
