#include "ftie.h"

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>


std::vector<uint8_t> ftie::physical_file_to_bytes_sequence(const char* filePath) {
  // https://stackoverflow.com/a/21802936/3649863
  // open the file:
  std::ifstream file(filePath, std::ios::in | std::ios::binary);
  // Stop eating new lines in binary mode!!!
  file.unsetf(std::ios::skipws);
  // get its size:
  std::streampos fileSize;

  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // reserve capacity
  std::vector<uint8_t> vec;
  vec.reserve(fileSize);

  // read the data:
  vec.insert(vec.begin(),
             std::istream_iterator<uint8_t>(file),
             std::istream_iterator<uint8_t>());

  return vec;
}

void ftie::bytes_sequence_to_physical_file(std::vector<uint8_t> bytes, const char* filePath) {
  std::ofstream outfile(filePath, std::ios::out | std::ios::binary);
  outfile.write((const char*)&bytes[0], bytes.size());
  outfile.close();
}

void ftie::bytes_sequence_padding(std::vector<uint8_t> bytes) {
  uint_fast32_t len_bts = bytes.size();
  uint_fast32_t len_rdt = len_bts * 2;
  float len_pixels = std::ceil(len_rdt / 3.0);
  //
  uint_fast16_t n_matrix = uint_fast16_t(std::ceil(std::sqrt(len_pixels)));
  if (n_matrix % 2 == 1)
      n_matrix += 1;
  //
  uint_fast32_t exp_len_pixels = n_matrix * n_matrix;
  uint_fast32_t exp_len_rdt = exp_len_pixels * 3;
  uint_fast32_t exp_len_bts = exp_len_rdt / 2;

  std::vector<uint8_t> pad(exp_len_bts - len_bts);
  bytes.insert(bytes.end(), pad.begin(), pad.end());
}

void ftie::bytes_sequence_stripping(std::vector<uint8_t> bytes) {
  std::vector<uint8_t>::iterator it = bytes.end() - 1;
  while (*(it - 1) == 0) it--;
  bytes.erase(it, bytes.end());
}

png::image<png::rgb_pixel> ftie::bytes_sequence_to_image(std::vector<uint8_t> bytes) {
  uint_fast32_t n = bytes.size();
  float np = std::ceil(n / 3.0);
  //
  uint_fast16_t N = uint_fast16_t(std::ceil(std::sqrt(np)));
  if (N % 2 == 1)
      N += 1;
  //
  png::image< png::rgb_pixel> image(N, N);
  for (uint_fast32_t i = 0; i < n; i+=3) {
    uint_fast16_t x = i / (3 * N);
    uint_fast16_t y = i / N;
    image[y][x].red = bytes[i];
    image[y][x].green = bytes[i + 1];
    image[y][x].blue = bytes[i + 2];
  }
  return image;
}

std::vector<uint8_t> ftie::image_to_bytes_sequence(png::image<png::rgb_pixel> image) {
  png::uint_16 N = image.get_height();
  uint_fast32_t n = N * N;
  std::vector<uint8_t> bytes(n);
  for (uint_fast32_t i = 0; i < n; i+=3) {
    uint_fast16_t x = i / (3 * N);
    uint_fast16_t y = i / N;
    bytes[i] = image[y][x].red;
    bytes[i + 1] = image[y][x].green;
    bytes[i  +2] = image[y][x].blue;
  }
  return bytes;
}

void ftie::encrypt(
  uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
  const char* plainfileFilepath, const char* cipherimageFilepath
) {
  std::vector<uint8_t> plainbytes = physical_file_to_bytes_sequence(plainfileFilepath);
  bytes_sequence_padding(plainbytes);

  bbs bbsBlock(p, q, s);
  std::vector<uint8_t> keystream = bbsBlock.generate_keystream(plainbytes.size());
  rt rtBlock;
  std::vector<uint8_t> cipherbytes = rtBlock.encrypt(plainbytes, keystream);

  png::image<png::rgb_pixel> plainimage = bytes_sequence_to_image(cipherbytes);

  acm acmBlock(a, b, n);
  png::image<png::rgb_pixel> cipherimage = acmBlock.encrypt(plainimage);

  cipherimage.write(cipherimageFilepath);
}

void ftie::decrypt(
  uint16_t p, uint16_t q, uint32_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
  const char* cipherimageFilepath, const char* plainfileFilepath
) {
  png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);

  acm acmBlock(a, b, n);
  png::image<png::rgb_pixel> plainimage = acmBlock.decrypt(cipherimage);

  std::vector<uint8_t> cipherbytes = image_to_bytes_sequence(plainimage);

  bbs bbsBlock(p, q, s);
  std::vector<uint8_t> keystream = bbsBlock.generate_keystream(cipherbytes.size() / 2);
  rt rtBlock;
  std::vector<uint8_t> plainbytes = rtBlock.decrypt(cipherbytes, keystream);

  bytes_sequence_stripping(plainbytes);
  bytes_sequence_to_physical_file(plainbytes, plainfileFilepath);
}
