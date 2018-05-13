#include "ftie.h"

#include "png++/png.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>


std::vector<uint8_t> ftie::read_plainfile(const char* filePath) {
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

void ftie::write_plainfile(std::vector<uint8_t> plainbytes, const char* filePath) {
  std::ofstream outfile(filePath, std::ios::out | std::ios::binary);
  outfile.write((const char*)&plainbytes[0], plainbytes.size());
  outfile.close();
}

std::vector<uint8_t> ftie::pad_bytes(std::vector<uint8_t> plainbytes) {
  uint_fast32_t len_bts = plainbytes.size();
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
  plainbytes.insert(plainbytes.end(), pad.begin(), pad.end());
  return plainbytes;
}

std::vector<uint8_t> ftie::strip_bytes(std::vector<uint8_t> plainbytes) {
  std::vector<uint8_t>::iterator it = plainbytes.end() - 1;
  while (*(it - 1) == 0) it--;
  plainbytes.erase(it, plainbytes.end());
  return plainbytes;
}

std::vector<std::vector<std::vector<uint8_t>>> ftie::bytes_to_matrix(std::vector<uint8_t> cipherbytes) {
  uint_fast32_t len_rdt = cipherbytes.size();
  float len_pixels = std::ceil(len_rdt / 3.0);
  //
  uint_fast16_t N = uint_fast16_t(std::ceil(std::sqrt(len_pixels)));
  if (N % 2 == 1)
      N += 1;
  //
  std::vector<std::vector<std::vector<uint8_t>>> plainmatrix(N, std::vector<std::vector<uint8_t>> (N, std::vector<uint8_t> (3)));
  uint_fast32_t i = 0;
  for(uint_fast32_t x = 0; x < N; x++) {
    for(uint_fast32_t y = 0; y < N; y++) {
      plainmatrix[x][y][0] = cipherbytes[i++];
      plainmatrix[x][y][1] = cipherbytes[i++];
      plainmatrix[x][y][2] = cipherbytes[i++];
    }
  }
  return plainmatrix;
}

std::vector<uint8_t> ftie::matrix_to_bytes(std::vector<std::vector<std::vector<uint8_t>>> plainmatrix) {
  uint_fast16_t len_matrix = plainmatrix.size();
  uint_fast16_t len_pixels = len_matrix * len_matrix;
  uint_fast32_t N = len_pixels * 3;
  std::vector<uint8_t> cipherbytes(N);
  uint_fast32_t i = 0;
  for(uint_fast32_t x = 0; x < len_matrix; x++) {
    for(uint_fast32_t y = 0; y < len_matrix; y++) {
      cipherbytes[i++] = plainmatrix[x][y][0];
      cipherbytes[i++] = plainmatrix[x][y][1];
      cipherbytes[i++] = plainmatrix[x][y][2];
    }
  }
  return cipherbytes;
}

png::image<png::rgb_pixel> ftie::matrix_to_image(std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix) {
  png::uint_16 N = ciphermatrix.size();
  png::image< png::rgb_pixel> cipherimage(N, N);
  for (png::uint_16 x = 0; x < N; x++) {
    for (png::uint_16 y = 0; y < N; y++) {
      // image[y][x] = png::rgb_pixel(0, 0, x);
      cipherimage[y][x] = png::rgb_pixel(
        ciphermatrix[x][y][0],
        ciphermatrix[x][y][1],
        ciphermatrix[x][y][2]
      );
    }
  }
  return cipherimage;
}

std::vector<std::vector<std::vector<uint8_t>>> ftie::image_to_matrix(png::image<png::rgb_pixel> cipherimage) {
  png::uint_16 N = cipherimage.get_height();
  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix(N, std::vector<std::vector<uint8_t>> (N, std::vector<uint8_t> (3)));
  for (png::uint_16 x = 0; x < N; x++) {
    for (png::uint_16 y = 0; y < N; y++) {
      ciphermatrix[x][y][0] = cipherimage[y][x].red;
      ciphermatrix[x][y][1] = cipherimage[y][x].green;
      ciphermatrix[x][y][2] = cipherimage[y][x].blue;
    }
  }
  return ciphermatrix;
}

void ftie::encrypt(
  uint16_t p, uint16_t q, uint64_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
  const char* plainfileFilepath, const char* cipherimageFilepath
) {
  std::vector<uint8_t> plainbytes = read_plainfile(plainfileFilepath);
  plainbytes = pad_bytes(plainbytes);

  bbs bbsBlock(p, q, s);
  std::vector<uint8_t> keystream = bbsBlock.generate_keystream(plainbytes.size());
  rt rtBlock;
  std::vector<uint8_t> cipherbytes = rtBlock.encrypt(plainbytes, keystream);

  std::vector<std::vector<std::vector<uint8_t>>> plainmatrix = bytes_to_matrix(cipherbytes);

  acm acmBlock(a, b, n);
  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix = acmBlock.encrypt(plainmatrix);

  png::image<png::rgb_pixel> cipherimage = matrix_to_image(ciphermatrix);
  cipherimage.write(cipherimageFilepath);
}

void ftie::decrypt(
  uint16_t p, uint16_t q, uint64_t s, uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
  const char* cipherimageFilepath, const char* plainfileFilepath
) {
  png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);
  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix = image_to_matrix(cipherimage);
  acm acmBlock(a, b, n);
  std::vector<std::vector<std::vector<uint8_t>>> plainmatrix = acmBlock.decrypt(ciphermatrix);

  std::vector<uint8_t> cipherbytes = matrix_to_bytes(plainmatrix);

  bbs bbsBlock(p, q, s);
  std::vector<uint8_t> keystream = bbsBlock.generate_keystream(cipherbytes.size());
  rt rtBlock;
  std::vector<uint8_t> plainbytes = rtBlock.decrypt(cipherbytes, keystream);

  plainbytes = strip_bytes(plainbytes);
  write_plainfile(plainbytes, plainfileFilepath);
}
