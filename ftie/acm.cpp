#include "acm.h"

#include "png++/png.hpp"

#include <cstdint>
#include <cmath>
#include <map>


std::vector<std::vector<uint16_t>> modular_matrix_multiplication(
  std::vector<std::vector<uint16_t>> A,
  std::vector<std::vector<uint16_t>> B,
  uint16_t m
) {
  uint16_t rows = A.size();
  uint16_t columns = B[0].size();
  uint16_t intersection = A.size();
  std::vector<std::vector<uint16_t>> Ret(rows, std::vector<uint16_t>(columns));
  for(uint16_t i = 0; i < rows; i++) {
    for(uint16_t j = 0; j < columns; j++) {
      Ret[i][j] = 0;
      for(uint16_t k = 0; k < intersection; k++) {
        uint16_t product = A[i][k] * B[k][j] % m;
        Ret[i][j] = (Ret[i][j] + product) % m;
      }
    }
  }
  return Ret;
}

std::vector<std::vector<uint16_t>> modular_matrix_exponentiation(
  std::vector<std::vector<uint16_t>> B,
  uint16_t e, uint16_t m
) {
  if (e == 0) {
    uint16_t N = B.size();
    std::vector<std::vector<uint16_t>> identity(N, std::vector<uint16_t>(N));
    for(uint16_t i = 0; i < N; i++) {
      for(uint16_t j = 0; j < N; j++) {
        if (i == j) {
          identity[i][j] = 1;
        } else {
          identity[i][j] = 0;
        }
      }
    }
    return identity;
  } else if (e % 2 == 1) {
    return modular_matrix_multiplication(
      B,
      modular_matrix_exponentiation(B, e - 1, m),
      m
    );
  }
  std::vector<std::vector<uint16_t>> D = modular_matrix_exponentiation(B, e / 2, m);
  return modular_matrix_multiplication(D, D, m);
}

uint16_t fibonacci(
  uint16_t n, uint16_t m=UINT16_MAX, uint16_t a=1
) {
  static std::map<uint16_t, uint16_t> cache{{0, 0}, {1, 1}};

  auto found = cache.find(n);
  if (found != std::end(cache)) {
    return found->second;
  }

  uint16_t result = ((a * fibonacci(n - 1, m, a) % m) + fibonacci(n - 2, m, a)) % m;
  cache[n] = result;
  return result;
}

std::vector<std::vector<std::vector<uint16_t>>> mapping_equal(
  uint16_t a, uint16_t b, uint16_t n, uint16_t N
) {
  std::vector<std::vector<std::vector<uint16_t>>> map(N, std::vector<std::vector<uint16_t>> (N, std::vector<uint16_t> (2)));
  for (uint16_t x = 0; x < N; x++) {
    for (uint16_t y = 0; y < N; y++) {
      map[x][y][0] = ((fibonacci(2 * n - 1, N, a) * x % N) + (fibonacci(2 * n, N, a) * y % N)) % N;
      map[x][y][1] = ((fibonacci(2 * n, N, a) * x % N) + (fibonacci(2 * n + 1, N, a) * y % N)) % N;
    }
  }
  return map;
}

std::vector<std::vector<std::vector<uint16_t>>> mapping_general(
  uint16_t a, uint16_t b, uint16_t n, uint16_t N
) {

  std::vector<std::vector<uint16_t>> A(2, std::vector<uint16_t> (2));
  A[0][0] = 1;
  A[0][1] = a % N;
  A[1][0] = b % N;
  A[1][1] = (1 + a * b) % N;

  std::vector<std::vector<uint16_t>> An = modular_matrix_exponentiation(A, 2, N);
  std::vector<std::vector<std::vector<uint16_t>>> map(N, std::vector<std::vector<uint16_t>> (N, std::vector<uint16_t> (2)));
  for (uint16_t x = 0; x < N; x++) {
    for (uint16_t y = 0; y < N; y++) {
      map[x][y][0] = ((An[0][0] * x % N)+(An[0][1] * y % N)) % N;
      map[x][y][1] = ((An[1][0] * x % N)+(An[1][1] * y % N)) % N;
    }
  }
  return map;
}

std::vector<std::vector<std::vector<uint16_t>>> get_map(
  uint16_t a, uint16_t b, uint16_t n, uint16_t N
) {
  std::vector<std::vector<std::vector<uint16_t>>> map;
  if (a == b)
    map = mapping_equal(a, b, n, N);
  else
    map = mapping_general(a, b, n, N);

  return map;
}

void validation(
  uint16_t a, uint16_t b, uint16_t n,
  png::image<png::rgb_pixel> img
) {
  if ((0 == a) || (0 == b) || (0 == n))
    throw "a or b or n is not in (0, inf)";
  if (img.get_height() != img.get_width())
    throw "image is not square";
  if (img.get_height() < 2)
    throw "img's N < 2";
}

uint16_t iroot_long(uint32_t n) {
  // https://en.wikipedia.org/wiki/Integer_square_root#Algorithm_using_Newton's_method
  float xk = n * 1.0;
  float xkp1 = (xk + (n / xk)) / 2.0;
  while (std::fabs(xkp1 - xk) >= 1) {
    xk = xkp1;
    xkp1 = (xk + (n / xk)) / 2.0;
  }
  return uint16_t(xkp1);
}

std::vector<uint32_t> get_map_deprecated(
  uint32_t a, uint32_t b, uint32_t n_t
) {
  uint16_t N = iroot_long(n_t) + 1;
  std::vector<uint32_t> M;
  for (uint16_t x = 0; x < N; x++) {
    for (uint16_t y = 0; y < N; y++) {
      uint16_t X = (x + (a * y % N)) % N;
      uint16_t Y = ((b * x % N) + (((a * b % N) + 1) * y % N)) % N;
      uint32_t S = N * X + Y;
      if ((0 <= S) && (S < n_t))
        M.push_back(S);
    }
  }
  return M;
}

void validation_deprecated(
  uint16_t a, uint16_t b, uint16_t n,
  std::vector<uint8_t> text
) {
  if ((0 == a) || (0 == b) || (0 == n))
    throw "a or b or n is not in (0, inf)";
  if (text.size() < 2)
    throw "text's N < 2";
}

namespace ftie {
  namespace acm {
    png::image<png::rgb_pixel> encrypt(
      uint16_t a, uint16_t b, uint16_t n,
      png::image<png::rgb_pixel> plainimage
    ) {
      validation(a, b, n, plainimage);
      uint16_t N = plainimage.get_height();
      std::vector<std::vector<std::vector<uint16_t>>> map = get_map(a, b, n, N);

      png::image< png::rgb_pixel> cipherimage(N, N);
      for (uint16_t x = 0; x < N; x++) {
        for (uint16_t y = 0; y < N; y++) {
          std::vector<uint16_t> pos = map[x][y];
          cipherimage[y][x] = plainimage[pos[1]][pos[0]];
        }
      }
      return cipherimage;
    }

    png::image<png::rgb_pixel> decrypt(
      uint16_t a, uint16_t b, uint16_t n,
      png::image<png::rgb_pixel> cipherimage
    ) {
      validation(a, b, n, cipherimage);
      uint16_t N = cipherimage.get_height();
      std::vector<std::vector<std::vector<uint16_t>>> map = get_map(a, b, n, N);
      png::image< png::rgb_pixel> plainimage(N, N);
      for (uint16_t x = 0; x < N; x++) {
        for (uint16_t y = 0; y < N; y++) {
          std::vector<uint16_t> pos = map[x][y];
          plainimage[pos[1]][pos[0]] = cipherimage[y][x];
        }
      }
      return plainimage;
    }
  }

  namespace deprecated {
    namespace acm {
      std::vector<uint8_t> encrypt(
        uint16_t a, uint16_t b, uint16_t n,
        std::vector<uint8_t> plaintext
      ) {
        validation_deprecated(a, b, n, plaintext);
        uint32_t n_t = plaintext.size();
        std::vector<uint8_t> ciphertext(n_t);
        std::vector<uint32_t> map = get_map_deprecated(a, b, n_t);
        for (uint16_t t = 0; t < n; t++) {
          for (uint32_t i = 0; i < n_t; i++) {
            ciphertext[i] = plaintext[map[i]];
          }
          plaintext = ciphertext;
        }
        return ciphertext;
      }

      std::vector<uint8_t> decrypt(
        uint16_t a, uint16_t b, uint16_t n,
        std::vector<uint8_t> ciphertext
      ) {
        validation_deprecated(a, b, n, ciphertext);
        uint32_t n_t = ciphertext.size();
        std::vector<uint8_t> plaintext(n_t);
        std::vector<uint32_t> map = get_map_deprecated(a, b, n_t);

        for (uint16_t t = 0; t < n; t++) {
          for (uint32_t i = 0; i < n_t; i++) {
            plaintext[map[i]] = ciphertext[i];
          }
          ciphertext = plaintext;
        }
        return plaintext;
      }
    }
  }
}
