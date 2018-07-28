#include "ftie.h"

#include "acm.h"
#include "bbs.h"
#include "rt.h"

#include "png++/png.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>
#include <iomanip>


std::vector<uint8_t> physical_file_to_bytes_sequence(const char* filePath) {
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

void bytes_sequence_to_physical_file(std::vector<uint8_t> bytes, const char* filePath) {
  std::ofstream outfile(filePath, std::ios::out | std::ios::binary);
  outfile.write((const char*)&bytes[0], bytes.size());
  outfile.close();
}

std::vector<uint8_t> bytes_sequence_padding(std::vector<uint8_t> bytes) {
  unsigned int x = 0x76543210;
  char *c = (char*) &x;

  if (*c != 0x10)
    throw "Underlying architecture is big endian";

  uint32_t len_bts = bytes.size();
  uint32_t len_rdt = (len_bts + 4) * 2;
  float len_pixels = std::ceil(len_rdt / 3.0);
  //
  uint16_t n_matrix = uint32_t(std::ceil(std::sqrt(len_pixels)));
  if (n_matrix % 2 == 1)
      n_matrix += 1;
  //
  uint32_t exp_len_pixels = n_matrix * n_matrix;
  uint32_t exp_len_rdt = exp_len_pixels * 3;
  uint32_t exp_len_bts = exp_len_rdt / 2;

  uint32_t dn = exp_len_bts - (len_bts + 4);
  std::vector<uint8_t> pad(dn);
  bytes.insert(bytes.end(), pad.begin(), pad.end());

  std::vector<uint8_t> dn_vector = {
    uint8_t(dn >> 24),
    uint8_t(dn >> 16),
    uint8_t(dn >> 8),
    uint8_t(dn)
  };

  bytes.insert(bytes.end(), dn_vector.begin(), dn_vector.end());

  return bytes;
}

std::vector<uint8_t> bytes_sequence_stripping(std::vector<uint8_t> bytes) {
  std::vector<uint8_t>::iterator it = bytes.end() - 1;

  uint32_t dn =
    (0x000000FF & *it--) |
    (0x0000FF00 & *it-- << 8) |
    (0x00FF0000 & *it-- << 16) |
    (0xFF000000 & *it << 24);

  bytes.erase(it - dn, bytes.end());
  return bytes;
}

png::image<png::rgb_pixel> bytes_sequence_to_image(std::vector<uint8_t> bytes) {
  uint32_t n = bytes.size();
  float np = std::ceil(n / 3.0);
  //
  uint16_t N = uint16_t(std::ceil(std::sqrt(np)));
  if (N % 2 == 1)
      N += 1;
  //
  png::image< png::rgb_pixel> image(N, N);
  for (uint32_t i = 0; i < n; i+=3) {
    uint16_t x = i / 3 % N;
    uint16_t y = i / 3 / N;
    image[y][x] = png::rgb_pixel(
      bytes[i],
      bytes[i + 1],
      bytes[i + 2]
    );
  }
  return image;
}

std::vector<uint8_t> image_to_bytes_sequence(png::image<png::rgb_pixel> image) {
  png::uint_16 N = image.get_height();
  uint32_t n = 3 * N * N;
  std::vector<uint8_t> bytes(n);
  for (uint32_t i = 0; i < n; i+=3) {
    uint16_t x = i / 3 % N;
    uint16_t y = i / 3 / N;
    bytes[i] = image[y][x].red;
    bytes[i + 1] = image[y][x].green;
    bytes[i  +2] = image[y][x].blue;
  }
  return bytes;
}

void print_byte_stream(std::vector<uint8_t> byteStream, std::string title, bool padded) {
  uint16_t n_matrix;
  if (padded) {
    n_matrix = uint32_t(std::ceil(std::sqrt((byteStream.size() * 2) / 3.0)));
  } else {
    uint32_t len_bts = byteStream.size();
    uint32_t len_rdt = (len_bts + 4) * 2;
    float len_pixels = std::ceil(len_rdt / 3.0);
    //
    n_matrix = uint32_t(std::ceil(std::sqrt(len_pixels)));
    if (n_matrix % 2 == 1)
    n_matrix += 1;
    //
  }
  size_t limit = n_matrix * 3;

  std::cout << title;

  size_t token = 0;
  for (size_t i = 0; i < byteStream.size(); i++) {
    if (token % limit == 0)
      std::cout << '\n';
    std::cout << std::setw(3) << int(byteStream[i]);
    std::cout << " ";
    token += 1;

    if (token % limit == 0)
      std::cout << '\n';
    std::cout << "   ";
    std::cout << " ";
    token += 1;
  }
  std::cout << '\n' << '\n';
}

void print_byte_stream(std::vector<uint8_t> byteStream, std::string title) {
  uint16_t n_matrix = uint32_t(std::ceil(std::sqrt(byteStream.size() / 3.0)));
  size_t limit = n_matrix * 3;

  std::cout << title;

  for (size_t i = 0; i < byteStream.size(); i++) {
    if (i % limit == 0)
      std::cout << '\n';
    std::cout << std::setw(3) << int(byteStream[i]);
    std::cout << " ";
  }
  std::cout << '\n' << '\n';
}

void print_image(png::image<png::rgb_pixel> image, std::string title) {
  std::cout << title << '\n';
  std::cout << "     | ";
  for (int i = 0; i < image.get_height(); i++) {
    std::cout << std::setw(8) << i;
    std::cout << "        ";
  }
  std::cout << '\n';
  for (int y = 0; y < image.get_height(); y++) {
    std::cout << std::setw(4) << y;
    std::cout << " | ";
    for (int x = 0; x < image.get_height(); x++) {
      std::cout << "(";
      std::cout << std::setw(3) << int(image[y][x].red);
      std::cout << ", ";
      std::cout << std::setw(3) << int(image[y][x].green);
      std::cout << ", ";
      std::cout << std::setw(3) << int(image[y][x].blue);
      std::cout << ") ";
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

namespace ftie {
  void encrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  ) {
    std::cout << "Encrypting " << plainfileFilepath << " into " << cipherimageFilepath << '\n';

    std::vector<uint8_t> plainbytes = physical_file_to_bytes_sequence(plainfileFilepath);

    print_byte_stream(plainbytes, "byte stream 1", false);

    plainbytes = bytes_sequence_padding(plainbytes);

    print_byte_stream(plainbytes, "byte stream 2", true);

    std::vector<uint8_t> keystream = ftie::bbs::generate_randoms(p, q, s, plainbytes.size());

    print_byte_stream(keystream, "keystream", true);

    std::vector<uint8_t> cipherbytes = ftie::rt::encrypt(plainbytes, keystream);

    print_byte_stream(cipherbytes, "byte stream 3");

    png::image<png::rgb_pixel> plainimage = bytes_sequence_to_image(cipherbytes);

    print_image(plainimage, "image");

    png::image<png::rgb_pixel> cipherimage = ftie::acm::encrypt(a, b, n, plainimage);

    print_image(cipherimage, "cipherimage");

    cipherimage.write(cipherimageFilepath);

    std::cout << "Done" << '\n';
  }

  void decrypt(
    uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  ) {
    std::cout << "Decrypting " << cipherimageFilepath << " into "<< plainfileFilepath << '\n';

    png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);

    print_image(cipherimage, "cipherimage");

    png::image<png::rgb_pixel> plainimage = ftie::acm::decrypt(a, b, n, cipherimage);

    print_image(plainimage, "image");

    std::vector<uint8_t> cipherbytes = image_to_bytes_sequence(plainimage);

    print_byte_stream(cipherbytes, "byte stream 3");

    std::vector<uint8_t> keystream = ftie::bbs::generate_randoms(p, q, s, cipherbytes.size() / 2);

    print_byte_stream(keystream, "keystream", true);

    std::vector<uint8_t> plainbytes = ftie::rt::decrypt(cipherbytes, keystream);

    print_byte_stream(plainbytes, "byte stream 2", true);

    plainbytes = bytes_sequence_stripping(plainbytes);

    print_byte_stream(plainbytes, "byte stream 1", false);

    bytes_sequence_to_physical_file(plainbytes, plainfileFilepath);

    std::cout << "Done" << '\n';
  }

  namespace deprecated {
    void encrypt(
      std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
      const char* plainfileFilepath, const char* cipherimageFilepath
    ) {
      std::cout << "Encrypting " << plainfileFilepath << " into " << cipherimageFilepath << '\n';

      std::vector<uint8_t> plainbytes = physical_file_to_bytes_sequence(plainfileFilepath);

      print_byte_stream(plainbytes, "byte stream 1", false);

      plainbytes = bytes_sequence_padding(plainbytes);

      print_byte_stream(plainbytes, "byte stream 2", true);

      std::vector<uint8_t> cipherbytes = ftie::rt::encrypt(plainbytes, keystream);

      print_byte_stream(cipherbytes, "byte stream 3");

      cipherbytes = ftie::deprecated::acm::encrypt(a, b, n, cipherbytes);

      print_byte_stream(cipherbytes, "byte stream 4");

      png::image<png::rgb_pixel> cipherimage = bytes_sequence_to_image(cipherbytes);

      print_image(cipherimage, "cipherimage");

      cipherimage.write(cipherimageFilepath);

      std::cout << "Done" << '\n';
    }

    void decrypt(
      std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
      const char* cipherimageFilepath, const char* plainfileFilepath
    ) {
      png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);

      print_image(cipherimage, "cipherimage");

      std::vector<uint8_t> cipherbytes = image_to_bytes_sequence(cipherimage);

      print_byte_stream(cipherbytes, "byte stream 4");

      cipherbytes = ftie::deprecated::acm::decrypt(a, b, n, cipherbytes);

      print_byte_stream(cipherbytes, "byte stream 3");

      std::vector<uint8_t> plainbytes = ftie::rt::decrypt(cipherbytes, keystream);

      print_byte_stream(plainbytes, "byte stream 2", true);

      plainbytes = bytes_sequence_stripping(plainbytes);

      print_byte_stream(plainbytes, "byte stream 1", false);

      bytes_sequence_to_physical_file(plainbytes, plainfileFilepath);

      std::cout << "Done" << '\n';
    }
  }
}
