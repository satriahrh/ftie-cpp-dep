// make experiment-build-e && make experiment-run-e
// Experiment e: BBS key size: every row of bbs_params.csv (prime sizes 16 to
// 1024 bits, modulus ~32 to ~2048 bits), with random a, b and n = 50. The
// deprecated pipeline does not use BBS and serves as a control.
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
    const std::string dum = "experiment/dum/rt";
    const std::string plainfile1 = dum + "_plainfile_1";
    const tools::ScratchPaths paths = {
      dum + "_plainfile_2", dum + "_cipherimage_1", dum + "_cipherimage_2", dum + "_roundtrip_1"
    };

    std::vector<uint8_t> plainbytes = tools::random_bytes(3210000);
    plainbytes[0] = 0x01;
    tools::write_bytes(plainfile1, plainbytes);
    tools::write_bit1_flipped(plainfile1.c_str(), paths.plain2.c_str());

    // Deprecated keystream sized to the plainfile, as in the pilot.
    std::vector<uint8_t> keystream = tools::random_bytes(plainbytes.size());
    const std::vector<tools::BbsParams> bbsRows = tools::read_bbs_params("experiment/bbs_params.csv");
    const uint16_t a = tools::random_acm_parameter();
    const uint16_t b = tools::random_acm_parameter();
    const uint16_t n = 50;

    std::cout << std::setprecision(tools::CSV_PRECISION);
    std::cout << tools::SWEEP_HEADER << ",modulus\n";
    for (const tools::BbsParams & bbs : bbsRows) {
      tools::PipelineResult dep = tools::measure_deprecated(keystream, a, b, n, plainfile1, paths);
      tools::PipelineResult cur = tools::measure_current(bbs.p, bbs.q, bbs.s, a, b, n, plainfile1, paths);
      tools::write_sweep_row(std::cout, bbs.bits, a, b, n, dep, cur);
      mpz_class m = bbs.p * bbs.q;
      std::cout << "," << m.get_str(10) << std::endl;
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
