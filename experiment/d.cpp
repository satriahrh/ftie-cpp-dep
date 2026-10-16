// make experiment-build-d && make experiment-run-d
// Experiment d: ACM matrix entry a = 1, 2185, ..., 63337 (30 steps of
// UINT16_MAX / 30) with b = a + 1 and n = 50, through mapping_general.
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
    const std::string dum = "experiment/dum/acm_any_ab";
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
    for (uint16_t a = 1; a <= max; a += inc) {
      const uint16_t b = a + 1;
      tools::PipelineResult dep = tools::measure_deprecated(keystream, a, b, n, plainfile1, paths);
      tools::PipelineResult cur = tools::measure_current(bbs.p, bbs.q, bbs.s, a, b, n, plainfile1, paths);
      tools::write_sweep_row(std::cout, bbs.bits, a, b, n, dep, cur);
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
