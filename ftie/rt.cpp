#include "rt.h"

#include <cstdint>
#include <random>
#include <vector>


std::vector<uint8_t> rt::encrypt(std::vector<uint8_t> plainbytes, std::vector<uint8_t> keystream) {
  int n = plainbytes.size();
  std::vector<uint8_t> cipherbytes(n * 2);
  std::random_device generateRandom;
  for (uint_fast32_t i = 0; i < n; i ++) {
    uint8_t random = generateRandom();
    uint8_t key = keystream[i % keystream.size()];
    cipherbytes[i * 2] = key + 2 * plainbytes[i] +  random;
    cipherbytes[i * 2 + 1] = 2 * key + plainbytes[i] + random;
  }
  return cipherbytes;
}

std::vector<uint8_t> rt::decrypt(std::vector<uint8_t> cipherbytes, std::vector<uint8_t> keystream) {
  int n = cipherbytes.size() / 2;
  std::vector<uint8_t> plainbytes(n);
  for (uint_fast32_t i = 0; i < n; i ++) {
    uint8_t key = keystream[i % keystream.size()];
    uint8_t a = cipherbytes[i * 2] - key;
    uint8_t b = cipherbytes[i * 2 + 1] - 2 * key;
    plainbytes[i] = a - b;
  }

  return plainbytes;
}
