#include "tools.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace tools {
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

    std::stringstream ss;
    ss << "/home/hafizhme/ftie-" << title << ".log";
    std::ofstream fout(ss.str());
    ss.clear();

    size_t token = 0;
    for (size_t i = 0; i < byteStream.size(); i++) {
      if (token % limit == 0)
        fout << '\n';
      fout << std::setw(3) << int(byteStream[i]);
      fout << " ";
      token += 1;

      if (token % limit == 0)
        fout << '\n';
      fout << "   ";
      fout << " ";
      token += 1;
    }
    fout << '\n' << '\n';
    fout.close();
  }

  void print_byte_stream(std::vector<uint8_t> byteStream, std::string title) {
    uint16_t n_matrix = uint32_t(std::ceil(std::sqrt(byteStream.size() / 3.0)));
    size_t limit = n_matrix * 3;

    std::stringstream ss;
    ss << "/home/hafizhme/ftie-" << title << ".log";
    std::ofstream fout(ss.str());
    ss.clear();

    for (size_t i = 0; i < byteStream.size(); i++) {
      if (i % limit == 0)
        fout << '\n';
      fout << std::setw(3) << int(byteStream[i]);
      fout << " ";
    }
    fout << '\n' << '\n';

    fout.close();
  }

  void print_image(png::image<png::rgb_pixel> image, std::string title) {
    std::stringstream ss;
    ss << "/home/hafizhme/ftie-" << title << ".log";
    std::ofstream fout(ss.str());
    ss.clear();

    fout << "     | ";
    for (int i = 0; i < image.get_height(); i++) {
      fout << std::setw(8) << i;
      fout << "        ";
    }
    fout << '\n';
    for (int y = 0; y < image.get_height(); y++) {
      fout << std::setw(4) << y;
      fout << " | ";
      for (int x = 0; x < image.get_height(); x++) {
        fout << "(";
        fout << std::setw(3) << int(image[y][x].red);
        fout << ", ";
        fout << std::setw(3) << int(image[y][x].green);
        fout << ", ";
        fout << std::setw(3) << int(image[y][x].blue);
        fout << ") ";
      }
      fout << '\n';
    }
    fout << '\n';

    fout.close();
  }

  // std::chrono::duration<double> chrono_encrypt(
  //   uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
  //   const char* plainfileFilepath, const char* cipherimageFilepath
  // ) {
  //   std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  //   ftie::encrypt(p, q, s, a, b, n, plainfileFilepath, cipherimageFilepath);
  //   std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
  //   return finish - start;
  // }
  //
  // std::chrono::duration<double> chrono_decrypt(
  //   uint16_t p, uint16_t q, uint32_t s, uint16_t a, uint16_t b, uint16_t n,
  //   const char* cipherimageFilepath, const char* plainfileFilepath
  // ) {
  //   std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  //   ftie::decrypt(p, q, s, a, b, n, cipherimageFilepath, plainfileFilepath);
  //   std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
  //   return finish - start;
  // }
  //
  // std::chrono::duration<double> chrono_deprecated_encrypt(
  //   std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
  //   const char* plainfileFilepath, const char* cipherimageFilepath
  // ) {
  //   std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  //   ftie::deprecated::encrypt(keystream, a, b, n, plainfileFilepath, cipherimageFilepath);
  //   std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
  //   return finish - start;
  // }
  //
  // std::chrono::duration<double> chrono_deprecated_decrypt(
  //   std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
  //   const char* cipherimageFilepath, const char* plainfileFilepath
  // ) {
  //   std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  //   ftie::deprecated::decrypt(keystream, a, b, n, cipherimageFilepath, plainfileFilepath);
  //   std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
  //   return finish - start;
  // }
}
