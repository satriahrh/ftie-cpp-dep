#include "acm.h"

#include <cstdint>
#include <map>


acm::acm(uint_fast16_t acmA, uint_fast16_t acmB, uint_fast16_t acmN) {
  if (acmA == 0)
    throw "a == 0";
  if (acmB == 0)
    throw "b == 0";
  if (acmN == 0)
    throw "n == 0";
  a = acmA;
  b = acmB;
  n = acmN;
  if (a == b) type = 0;
  else type = 1;
}

std::vector<std::vector<uint_fast16_t>> modular_matrix_multiplication(std::vector<std::vector<uint_fast16_t>> A, std::vector<std::vector<uint_fast16_t>> B, uint_fast16_t m) {
  uint_fast16_t rows = A.size();
  uint_fast16_t columns = B[0].size();
  uint_fast16_t intersection = A.size();
  std::vector<std::vector<uint_fast16_t>> Ret(rows, std::vector<uint_fast16_t>(columns));
  for(uint_fast16_t i = 0; i < rows; i++) {
    for(uint_fast16_t j = 0; j < columns; j++) {
      Ret[i][j] = 0;
      for(uint_fast16_t k = 0; k < intersection; k++) {
        uint_fast16_t product = A[i][k] * B[k][j] % m;
        Ret[i][j] = (Ret[i][j] + product) % m;
      }
    }
  }
  return Ret;
}

std::vector<std::vector<uint_fast16_t>> modular_matrix_exponentiation(std::vector<std::vector<uint_fast16_t>> B, uint_fast16_t e, uint_fast16_t m) {
  if (e == 0) {
    uint_fast16_t N = B.size();
    std::vector<std::vector<uint_fast16_t>> identity(N, std::vector<uint_fast16_t>(N));
    for(uint_fast16_t i = 0; i < N; i++) {
      for(uint_fast16_t j = 0; j < N; j++) {
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
  std::vector<std::vector<uint_fast16_t>> D = modular_matrix_exponentiation(B, e / 2, m);
  return modular_matrix_multiplication(D, D, m);
}

uint_fast16_t fibonacci(uint_fast16_t n, uint_fast16_t m=UINT_FAST16_MAX, uint_fast16_t a=1) {
  static std::map<uint_fast16_t, uint_fast16_t> cache{{0, 0}, {1, 1}};

  auto found = cache.find(n);
  if (found != std::end(cache)) {
    return found->second;
  }

  uint_fast16_t result = ((a * fibonacci(n - 1, m, a) % m) + fibonacci(n - 2, m, a)) % m;
  cache[n] = result;
  return result;
}

std::vector<std::vector<std::vector<uint_fast16_t>>> acm::mapping_equal(uint_fast16_t N) {
  std::vector<std::vector<std::vector<uint_fast16_t>>> map(N, std::vector<std::vector<uint_fast16_t>> (N, std::vector<uint_fast16_t> (2)));
  for (uint_fast16_t x = 0; x < N; x++) {
    for (uint_fast16_t y = 0; y < N; y++) {
      map[x][y][0] = ((fibonacci(2 * n - 1, N, a) * x % N) + (fibonacci(2 * n, N, a) * y % N)) % N;
      map[x][y][1] = ((fibonacci(2 * n, N, a) * x % N) + (fibonacci(2 * n + 1, N, a) * y % N)) % N;
    }
  }
  return map;
}

std::vector<std::vector<std::vector<uint_fast16_t>>> acm::mapping_general(uint_fast16_t N) {
  std::vector<std::vector<uint_fast16_t>> A = {
    {1, a},
    {b, 1 + a * b}
  };
  std::vector<std::vector<uint_fast16_t>> An = modular_matrix_exponentiation(A, 2, N);
  std::vector<std::vector<std::vector<uint_fast16_t>>> map(N, std::vector<std::vector<uint_fast16_t>> (N, std::vector<uint_fast16_t> (2)));
  for (uint_fast16_t x = 0; x < N; x++) {
    for (uint_fast16_t y = 0; y < N; y++) {
      map[x][y][0] = ((An[0][0] * x % N)+(An[0][1] * y % N)) % N;
      map[x][y][1] = ((An[1][0] * x % N)+(An[1][1] * y % N)) % N;
    }
  }
  return map;
}

std::vector<std::vector<std::vector<uint_fast16_t>>> acm::get_map(uint_fast16_t N) {
  std::vector<std::vector<std::vector<uint_fast16_t>>> map;
  if (type == 0)
    map = mapping_equal(N);
  else
    map = mapping_general(N);

  return map;
}

std::vector<std::vector<std::vector<uint8_t>>> acm::encrypt(std::vector<std::vector<std::vector<uint8_t>>> plainmatrix) {
  uint_fast16_t N = plainmatrix.size();
  std::vector<std::vector<std::vector<uint_fast16_t>>> map = get_map(N);
  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix(N, std::vector<std::vector<uint8_t>> (N, std::vector<uint8_t> (3)));
  for (uint_fast16_t x = 0; x < N; x++) {
    for (uint_fast16_t y = 0; y < N; y++) {
      std::vector<uint_fast16_t> pos = map[x][y];
      ciphermatrix[x][y][0] = plainmatrix[pos[0]][pos[1]][0];
      ciphermatrix[x][y][1] = plainmatrix[pos[0]][pos[1]][1];
      ciphermatrix[x][y][2] = plainmatrix[pos[0]][pos[1]][2];
    }
  }
  return ciphermatrix;
}

std::vector<std::vector<std::vector<uint8_t>>> acm::decrypt(std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix) {
  uint_fast16_t N = ciphermatrix.size();
  std::vector<std::vector<std::vector<uint_fast16_t>>> map = get_map(N);
  std::vector<std::vector<std::vector<uint8_t>>> plainmatrix(N, std::vector<std::vector<uint8_t>>(N, std::vector<uint8_t>(3)));
  for (uint_fast16_t x = 0; x < N; x++) {
    for (uint_fast16_t y = 0; y < N; y++) {
      std::vector<uint_fast16_t> pos = map[x][y];
      plainmatrix[pos[0]][pos[1]][0] = ciphermatrix[x][y][0];
      plainmatrix[pos[0]][pos[1]][1] = ciphermatrix[x][y][1];
      plainmatrix[pos[0]][pos[1]][2] = ciphermatrix[x][y][2];
    }
  }
  return plainmatrix;
}
