#ifndef TOOLS_H
#define TOOLS_H

#include "png++/png.hpp"

#include <cstdint>
#include <vector>


namespace tools {
  void print_byte_stream(std::vector<uint8_t> byteStream, std::string title, bool padded);
  void print_byte_stream(std::vector<uint8_t> byteStream, std::string title);
  void print_image(png::image<png::rgb_pixel> image, std::string title);

  // double chrono_encrypt(
  //   uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
  //   const char* plainfileFilepath, const char* cipherimageFilepath
  // );
  //
  // double chrono_decrypt(
  //   uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
  //   const char* cipherimageFilepath, const char* plainfileFilepath
  // );
  //
  // double chrono_deprecated_encrypt(
  //   std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
  //   const char* plainfileFilepath, const char* cipherimageFilepath
  // );
  //
  // double chrono_deprecated_decrypt(
  //   std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
  //   const char* cipherimageFilepath, const char* plainfileFilepath
  // );
}

#endif
