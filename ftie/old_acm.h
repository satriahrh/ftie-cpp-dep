#ifndef OLD_ACM_H
#define OLD_ACM_H

#include <cstdint>
#include <vector>


class old_acm {
private:
  std::vector<uint_fast32_t> get_map(uint_fast16_t a, uint_fast16_t b, uint_fast32_t l);
public:
  std::vector<uint8_t> encrypt(uint_fast16_t a, uint_fast16_t b, std::vector<uint8_t> plainbytes);
  std::vector<uint8_t> decrypt(uint_fast16_t a, uint_fast16_t b, std::vector<uint8_t> cipherbytes);
};

#endif
