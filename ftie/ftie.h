#ifndef FTIE_H
#define FTIE_H

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <vector>


class ftie {
private:
  std::vector<uint8_t> physical_file_to_bytes_sequence(const char* filePath);
  std::vector<uint8_t> bytes_sequence_padding(std::vector<uint8_t> plainbytes);
  png::image<png::rgb_pixel> bytes_sequence_to_image(std::vector<uint8_t> cipherbytes);

  std::vector<uint8_t> image_to_bytes_sequence(png::image<png::rgb_pixel> cipherimage);
  std::vector<uint8_t> bytes_sequence_stripping(std::vector<uint8_t> plainbytes);
  void bytes_sequence_to_physical_file(std::vector<uint8_t>, const char* filePath);

public:
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
