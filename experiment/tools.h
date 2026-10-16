#ifndef TOOLS_H
#define TOOLS_H

#include <chrono>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gmpxx.h>

namespace tools {
  struct BbsParams {
    unsigned int bits;
    mpz_class p, q, s;
  };

  std::vector<BbsParams> read_bbs_params(const char* csvFilepath);

  // Timed encrypt/decrypt runs per configuration; the median is reported.
  // Set to 1 for the paper's run (F11 in experiment/METHODOLOGY.md); 3 would
  // roughly double total run time.
  constexpr int TIMING_REPEATS = 1;

  // Scratch files one measurement writes. plain2 must already exist
  // (see write_bit1_flipped); the others are overwritten.
  struct ScratchPaths {
    std::string plain2, cipher1, cipher2, roundtrip;
  };

  // Everything measured for one pipeline on one plainfile:
  //   enc_s/dec_s: median over TIMING_REPEATS of Enc(P1)->C1 and Dec(C1),
  //                each decryption checked byte-for-byte against P1;
  //   npcr, npcr_ch, uaci: C1 vs a second Enc(P1);
  //   entropy, chi2: C1;
  //   avalanche: C1 vs Enc(P2).
  struct PipelineResult {
    double enc_s, dec_s, npcr, npcr_ch, uaci, entropy, chi2, avalanche;
  };

  // Throw std::runtime_error on a round-trip mismatch.
  PipelineResult measure_current(
    const mpz_class & p, const mpz_class & q, const mpz_class & s,
    uint16_t a, uint16_t b, uint16_t n,
    const std::string & plainfile1, const ScratchPaths & paths
  );
  PipelineResult measure_deprecated(
    const std::vector<uint8_t> & keystream,
    uint16_t a, uint16_t b, uint16_t n,
    const std::string & plainfile1, const ScratchPaths & paths
  );

  // P2 for the avalanche pair: a copy of src with bit 1 of byte 0 flipped
  // (the a-e convention 0x01 -> 0x03, requirement M4).
  void write_bit1_flipped(const char * srcFilepath, const char * dstFilepath);
  bool files_equal(const char * path1, const char * path2);

  std::vector<uint8_t> random_bytes(size_t n);
  void write_bytes(const std::string & filepath, const std::vector<uint8_t> & bytes);
  // ACM rejects a == 0 or b == 0, so random ACM parameters are drawn from 1..65535.
  uint16_t random_acm_parameter();

  // CSV layout shared by the a-e sweeps: every row carries its full key
  // (prime_bits identifies the bbs_params.csv row holding p, q, s), then
  // the PipelineResult metrics for the deprecated and current pipelines.
  extern const char * const SWEEP_HEADER;
  void write_sweep_row(
    std::ostream & out, unsigned int primeBits, uint16_t a, uint16_t b, uint16_t n,
    const PipelineResult & dep, const PipelineResult & cur
  );

  // Significant digits for every result CSV: entropy's shortfall from 8 is
  // ~1e-5, so the stream default of 6 would round it away.
  constexpr int CSV_PRECISION = 10;

  // Fraction of differing bits over the decoded pixel bytes of two
  // equally sized cipherimages (not the PNG file bytes, whose identical
  // container framing biases the value below 0.5).
  double calculate_avalanche(
    const char * cipherimage1Filepath,
    const char * cipherimage2Filepath
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
  // actual width/height rather than a fixed table entry. NPCR and UACI
  // account for RT's byte pairs: both bytes of a pair change together
  // between two encryptions, so the 3WH channel bytes are 1.5WH
  // independent samples (F2 in experiment/METHODOLOGY.md).
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

  // Variance of one RT pair's UACI term (|dC_2i| + |dC_2i+1|) / (2*255),
  // exact, by enumerating c1, c1', d in [0,256)^3 with c2 = c1 + d,
  // c2' = c1' + d (mod 256), d = K - P' uniform. The single-byte variance
  // from the same enumeration is 0.0559930 (Wu et al.'s 0.05599); the
  // within-pair correlation is 0.400.
  constexpr double UACI_PAIR_VARIANCE = 0.0391966337;

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
    const std::vector<uint8_t> & keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* plainfileFilepath, const char* cipherimageFilepath
  );

  std::chrono::duration<double> chrono_deprecated_decrypt(
    const std::vector<uint8_t> & keystream, uint16_t a, uint16_t b, uint16_t n,
    const char* cipherimageFilepath, const char* plainfileFilepath
  );
}

#endif
