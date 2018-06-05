#ifndef RT_H
#define RT_H

#include <cstdint>
#include <vector>


namespace ftie {
  namespace rt {
    std::vector<uint8_t> encrypt(std::vector<uint8_t> plainbytes, std::vector<uint8_t> keystream);
    std::vector<uint8_t> decrypt(std::vector<uint8_t> cipherbytes, std::vector<uint8_t> keystream);
  }
}

#endif
