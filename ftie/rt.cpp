#include "rt.h"

#include <cstdint>
#include <random>
#include <vector>


namespace ftie {
  namespace rt {
    std::vector<uint8_t> encrypt(
      std::vector<uint8_t> plainbytes, std::vector<uint8_t> keystream
    ) {
      int n = plainbytes.size();
      int n_k = keystream.size();
      std::vector<uint8_t> cipherbytes(n * 2);
      std::random_device generateRandom;
      for (uint32_t i = 0; i < n; i ++) {
        uint8_t random = generateRandom();
        cipherbytes[i * 2] = keystream[i % n_k] + 2 * plainbytes[i] +  random;
        cipherbytes[i * 2 + 1] = 2 * keystream[i % n_k] + plainbytes[i] + random;
      }
      return cipherbytes;
    }

    std::vector<uint8_t> decrypt(
      std::vector<uint8_t> cipherbytes, std::vector<uint8_t> keystream
    ) {
      int n = cipherbytes.size() / 2;
      int n_k = keystream.size();
      std::vector<uint8_t> plainbytes(n);
      for (uint32_t i = 0; i < n; i ++) {
        uint8_t a = cipherbytes[i * 2] - keystream[i % n_k];
        uint8_t b = cipherbytes[i * 2 + 1] - 2 * keystream[i % n_k];
        plainbytes[i] = a - b;
      }

      return plainbytes;
    }
  }
}
