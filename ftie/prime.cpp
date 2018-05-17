#include "prime.h"

#include "suplementary.h"

#include <cstdint>
#include <fstream>


prime::prime() {
  std::string line;
  std::ifstream primeFile("./data/primes_2_bytes_for_p_q");
  while (std::getline(primeFile, line)) {
    primes.insert(std::stoi(line));
  }
  primeFile.close();
}

bool prime::is_prime(uint16_t n) {
  bool verdict = primes.find(n) != primes.end();
  return verdict;
}
