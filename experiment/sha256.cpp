// SHA-256, ported from the public-domain reference implementation by
// Brad Conte (https://github.com/B-Con/crypto-algorithms, sha256.c,
// "This code is released into the public domain free of any restrictions").
// Adapted to an incremental init/update/final context so large files can
// be streamed in chunks instead of loaded into memory at once.
#include "sha256.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools {
  namespace {
    constexpr uint32_t K[64] = {
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    inline uint32_t rotr(uint32_t x, uint32_t n) {
      return (x >> n) | (x << (32 - n));
    }

    struct Sha256Ctx {
      uint8_t buffer[64];
      size_t bufferLen = 0;
      uint64_t totalLen = 0;
      uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
      };
    };

    void transform(Sha256Ctx & ctx, const uint8_t block[64]) {
      uint32_t w[64];
      for (int i = 0; i < 16; i++) {
        w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
               (uint32_t(block[i * 4 + 2]) << 8) | uint32_t(block[i * 4 + 3]);
      }
      for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
      }

      uint32_t a = ctx.state[0], b = ctx.state[1], c = ctx.state[2], d = ctx.state[3];
      uint32_t e = ctx.state[4], f = ctx.state[5], g = ctx.state[6], h = ctx.state[7];

      for (int i = 0; i < 64; i++) {
        uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
      }

      ctx.state[0] += a; ctx.state[1] += b; ctx.state[2] += c; ctx.state[3] += d;
      ctx.state[4] += e; ctx.state[5] += f; ctx.state[6] += g; ctx.state[7] += h;
    }

    void update(Sha256Ctx & ctx, const uint8_t * data, size_t len) {
      ctx.totalLen += len;
      while (len > 0) {
        size_t take = std::min(len, size_t(64) - ctx.bufferLen);
        std::memcpy(ctx.buffer + ctx.bufferLen, data, take);
        ctx.bufferLen += take;
        data += take;
        len -= take;
        if (ctx.bufferLen == 64) {
          transform(ctx, ctx.buffer);
          ctx.bufferLen = 0;
        }
      }
    }

    std::string finalize(Sha256Ctx & ctx) {
      uint64_t bitLen = ctx.totalLen * 8;

      uint8_t pad = 0x80;
      update(ctx, &pad, 1);

      uint8_t zero = 0x00;
      while (ctx.bufferLen != 56) {
        update(ctx, &zero, 1);
      }

      uint8_t lenBytes[8];
      for (int i = 0; i < 8; i++) {
        lenBytes[i] = uint8_t(bitLen >> (56 - 8 * i));
      }
      // bypass update()'s totalLen accounting for the length suffix itself
      std::memcpy(ctx.buffer + ctx.bufferLen, lenBytes, 8);
      transform(ctx, ctx.buffer);

      static const char * hex = "0123456789abcdef";
      std::string digest;
      digest.reserve(64);
      for (int i = 0; i < 8; i++) {
        for (int shift = 24; shift >= 0; shift -= 8) {
          uint8_t byte = uint8_t(ctx.state[i] >> shift);
          digest.push_back(hex[byte >> 4]);
          digest.push_back(hex[byte & 0xF]);
        }
      }
      return digest;
    }
  }

  std::string sha256_bytes(const uint8_t* data, size_t len) {
    Sha256Ctx ctx;
    update(ctx, data, len);
    return finalize(ctx);
  }

  std::string sha256_file(const char* path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
      std::stringstream ss;
      ss << "sha256_file: could not open " << path;
      throw std::runtime_error(ss.str());
    }

    Sha256Ctx ctx;
    std::vector<uint8_t> chunk(65536);
    while (file) {
      file.read(reinterpret_cast<char*>(chunk.data()), chunk.size());
      std::streamsize got = file.gcount();
      if (got > 0) {
        update(ctx, chunk.data(), size_t(got));
      }
    }
    return finalize(ctx);
  }
}
