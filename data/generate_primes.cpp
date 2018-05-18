// to generate prime number in binary (for memory optimation)

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

void generate(
  const char * primeTextFilePath,
  const char * primeBinaryFilePath
) {
  std::vector<uint16_t> primes;
  std::string line;
  std::ifstream primeTextFile(primeTextFilePath);
  while (std::getline(primeTextFile, line))
    primes.push_back(std::stoi(line));
  primeTextFile.close();

  std::ofstream primeBinaryFile(primeBinaryFilePath, std::ios::out | std::ios::binary);
  for (int i = 0; i < primes.size(); i++)
    primeBinaryFile.write(reinterpret_cast<const char *>(&primes[i]), sizeof(primes[i]));
  primeBinaryFile.close();
}

int main(int argc, char const *argv[]) {
  // 1 bytees prime is mean to be in local development machine
  // highly not recommended for production
  generate("./data/primes_1.txt", "./data/primes_1.bin");
  generate("./data/primes_1_3mod4.txt", "./data/primes_1_3mod4.bin");

  // 2 bytees prime is mean to be in experimental machine
  // not recommended for production
  generate("./data/primes_2.txt", "./data/primes_2.bin");
  generate("./data/primes_2_3mod4.txt", "./data/primes_2_3mod4.bin");

  // TODO generate 4 bytes prime for production
  // TODO support for 4 bytes prime

  return 0;
}
