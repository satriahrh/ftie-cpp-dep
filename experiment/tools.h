#ifndef TOOLS_H
#define TOOLS_H

#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

#include <gmpxx.h>

namespace tools {
  struct BbsParams {
    unsigned int bits;
    mpz_class p, q, s;
  };

  std::vector<BbsParams> read_bbs_params(const char* csvFilepath);

  double calculate_avalanche(
    const char * path_1,
    const char * path_2
  );
  double calculate_entropy(
    const char * cipherimageFilepath
  );
  double calculate_npcr(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  );

  // Per-channel differing-pixel fraction (mean of R,G,B, each [0,1]),
  // distinct from calculate_npcr's any-channel-differs fraction.
  double calculate_npcr_channel(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  );

  // Returns a percentage in [0,100] (not a [0,1] fraction like
  // calculate_npcr/calculate_avalanche), to match the literature constants
  // used by uaci_critical_range below.
  double calculate_uaci(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
  );

  // Pooled R+G+B 256-bin histogram, same as calculate_entropy. df is
  // always 255 regardless of image size.
  double calculate_chi_square(
    const char * cipherimageFilepath
  );

  // M7 plainfile baseline: histogram raw file bytes directly (the
  // plainfile is not a PNG), not the pooled-pixel-channel bytes of a
  // cipherimage.
  double calculate_entropy_bytes(
    const char * filepath
  );
  double calculate_chi_square_bytes(
    const char * filepath
  );

  // M3 per-image critical values, parameterized by the cipherimage's
  // actual width/height rather than a fixed table entry.
  double npcr_critical_value(uint32_t width, uint32_t height, double z);
  std::pair<double, double> uaci_critical_range(uint32_t width, uint32_t height, double z);
  double entropy_critical_value(uint32_t width, uint32_t height, double chiSquareCritical);

  constexpr double Z_ONE_SIDED_ALPHA_05 = 1.645;
  constexpr double Z_ONE_SIDED_ALPHA_01 = 2.326;
  constexpr double Z_TWO_SIDED_ALPHA_05 = 1.96;
  constexpr double Z_TWO_SIDED_ALPHA_01 = 2.576;

  // Fixed chi-square critical values at df=255 (256-bin histogram, df does
  // not depend on image size).
  constexpr double CHI_SQUARE_CRITICAL_DF255_ALPHA_05 = 293.25;
  constexpr double CHI_SQUARE_CRITICAL_DF255_ALPHA_01 = 310.46;

  // M2 validity floor: each of 256 bins needs expected count >= 5, i.e.
  // n >= 1280. Not auto-enforced by calculate_chi_square(_bytes); callers
  // must check sample size before treating a chi2 value as meaningful.
  constexpr uint64_t CHI_SQUARE_MIN_SAMPLES = 1280;
  std::chrono::duration<double> chrono_encrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_decrypt(
    mpz_class p, mpz_class q, mpz_class s, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );

  std::chrono::duration<double> chrono_deprecated_encrypt(
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_deprecated_decrypt(
    std::vector<uint8_t> keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
}

#endif
