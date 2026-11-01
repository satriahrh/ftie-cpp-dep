// make experiment-build-c && make experiment-run-c
// Experiment c: ACM matrix entry a = b = 1, 2185, ..., 63337 (30 steps of
// UINT16_MAX / 30) with n = 50, through the closed-form Fibonacci mapping.
// 3.21 MB random plainfile (byte 0 = 0x01; P2 flips bit 1), last bbs_params.csv
// row unless noted. Prints its own CSV header, then one row per sweep value,
// to stdout; run_experiments.sh captures it. See experiment/METHODOLOGY.md §5.
#include "../experiment/tools.h"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <gmpxx.h>


int main() {
  try {
    const std::string dum = "experiment/dum/acm_equal_a";
    const std::string plainfile1 = dum + "_plainfile_1";
    const tools::ScratchPaths paths = {
      dum + "_plainfile_2", dum + "_cipherimage_1", dum + "_cipherimage_2", dum + "_roundtrip_1"
    };

    std::vector<uint8_t> plainbytes = tools::random_bytes(3210000);
    plainbytes[0] = 0x01;
    tools::write_bytes(plainfile1, plainbytes);
    tools::write_bit1_flipped(plainfile1.c_str(), paths.plain2.c_str());

    std::vector<uint8_t> keystream = tools::random_bytes(10000000);
    const tools::BbsParams bbs = tools::read_bbs_params("experiment/bbs_params.csv").back();
    const uint16_t n = 50;
    const uint16_t inc = UINT16_MAX / 30;
    const uint16_t max = UINT16_MAX - inc;

    std::cout << std::setprecision(tools::CSV_PRECISION);
    std::cout << tools::SWEEP_HEADER << "\n";
    // One running BBS seed for the whole sweep, starting at the key's s.
    mpz_class seed = bbs.s;
    for (uint16_t a = 1; a <= max; a += inc) {
      const uint16_t b = a;
      tools::PipelineResult prev = tools::measure_previous(keystream, a, b, n, plainfile1, paths);
      tools::ProposedSeeds seeds;
      tools::PipelineResult prop = tools::measure_proposed(bbs.p, bbs.q, seed, a, b, n, plainfile1, paths, seeds);
      tools::write_sweep_row(std::cout, bbs.bits, a, b, n, prev, prop, seeds);
      std::cout << std::endl;
    }
  } catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const char * msg) {
    std::cerr << msg << '\n';
    return 1;
  }
  return 0;
}
