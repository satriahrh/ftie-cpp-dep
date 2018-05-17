#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_set>

void generate(uint16_t pFrom, uint16_t pUntil, const char * pqsFilePath="", const char * primeFilePath="") {
  std::set<uint16_t> primes;
  std::string line;
  primeFilePath = primeFilePath == ""? "./data/primes_2_bytes_for_p_q" : primeFilePath;
  std::ifstream primeFile(primeFilePath);
  while (std::getline(primeFile, line)) {
    primes.insert(std::stoi(line));
  }
  primeFile.close();

  std::set<uint16_t>::iterator ipFrom = primes.find(pFrom);
  std::set<uint16_t>::iterator ipUntil = primes.find(pUntil);
  std::set<uint16_t>::iterator end = primes.end();
  std::set<uint16_t>::iterator beforeEnd = primes.end()--;

  pqsFilePath = pqsFilePath == ""? "./data/s_for_p_and_q" : pqsFilePath;
  std::stringstream ss;
  ss << pqsFilePath << "_" << pFrom << "_" << pUntil;
  std::ofstream pqsFile(ss.str().c_str(), std::ios::binary);

  for (std::set<uint16_t>::iterator p = ipFrom; p != ipUntil; p++) {
    for (std::set<uint16_t>::iterator q = std::next(p, 1); q != end; q++) {
      uint32_t m = *p * *q;
      uint16_t p_temp = *p;
      uint16_t q_temp = *q;
      std::cout << *p << " " << *q << '\n';
      pqsFile.write(reinterpret_cast<const char *>(&p_temp), 2);
      pqsFile.write(reinterpret_cast<const char *>(&q_temp), 2);
      // std::cout << m << '\n';
      for (uint32_t s = 2; s < m; s++) {
        if (std::gcd(s, m) == 1) {
          pqsFile.write(reinterpret_cast<const char *>(&s), 4);
          // std::cout << s << '\n';
        }
      }
      m = 0;
      pqsFile.write(reinterpret_cast<const char *>(&m), 4);
      // std::cout << m << '\n';
    }
  }
  pqsFile.close();
}

void read(const char * pqsFilePath="") {
  // // THIS CODE IS FOR READING PURPOSE THAT YOU WILL NEED FOR KPA
  pqsFilePath = pqsFilePath == ""? "./data/s_for_p_and_q" : pqsFilePath;
  std::ifstream pqsFile(pqsFilePath, std::ios::binary);
  uint32_t buffer;
  uint32_t m;
  uint16_t p;
  uint16_t q;
  uint32_t s;
  bool isPQ = true;
  while (pqsFile.read((char*)&buffer, 4)) {
    if (buffer == 0) {
      isPQ = true;
      // std::cout << 0 << '\n';
    } else if (isPQ) {
      p = (uint16_t)(buffer & 0x0000FFFF);
      q = (uint16_t)((buffer & 0xFFFF0000) >> 16);
      m = p * q;
      isPQ = false;
      std::cout << p << " " << q << '\n';
      // std::cout << m << '\n';
    } else {
      s = buffer;
      // DO KPA HERE
      // you can create bbs using p, q, and s here
      // std::cout << s << '\n';
    }
  }
  pqsFile.close();
}

int main(int argc, char const *argv[]) {
  uint16_t pFrom = atoi(argv[1]);
  uint16_t pUntil = atoi(argv[2]);
  // argv[1] for PQS FILE PATH
  // argv[2] for PRIME FILE PATH
  if (argc == 3) {
    generate(pFrom, pUntil);
  } else if (argc == 4) {
    generate(pFrom, pUntil, argv[3]);
  } if (argc == 5) {
    generate(pFrom, pUntil, argv[3], argv[4]);
  }
  return 0;
}
