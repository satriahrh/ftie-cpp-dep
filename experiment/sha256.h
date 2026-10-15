#ifndef SHA256_H
#define SHA256_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace tools {
  // 64-char lowercase hex digest. Throws std::runtime_error if the file
  // can't be opened. Streams the file in fixed-size chunks rather than
  // loading it entirely into memory.
  std::string sha256_file(const char* path);

  // SHA-256 over an in-memory buffer; sha256_file delegates to this per chunk.
  std::string sha256_bytes(const uint8_t* data, size_t len);
}

#endif
