// g++ -o bin/kpa-brute-force -std=c++17 ftie/*.h ftie/*.cpp pengujian/kpa_bf.cpp `libpng-config --ldflags`
// ./bin/kpa-brute-force 3 7 20

#include "../ftie/acm.h"
#include "../ftie/bbs.h"
#include "../ftie/ftie.h"
#include "../ftie/rt.h"
#include "../ftie/prime.h"

#include "png++/png.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

struct attackRet {
  double elapsed;
  uint16_t p;
  uint16_t q;
  uint64_t s;
};

uint_fast16_t A = 1;
uint_fast16_t B = 1;
uint_fast16_t N = 4;

// template<typename Out>
// void split(const string &s, char delim, Out result) {
//     stringstream ss(s);
//     string item;
//     while (getline(ss, item, delim)) {
//         *(result++) = item;
//     }
// }

// vector<string> split(const string &s, char delim) {
//     vector<string> elems;
//     split(s, delim, back_inserter(elems));
//     return elems;
// }

// vector<uint64_t> generate_s(uint64_t m) {
//   vector<uint64_t> ret;
//   bool * coPrime = new bool[m];
//   prime prm(m);
//   uint64_t currentPrime = 2;
//   for(uint64_t i = 2; i < m; i++) {
//     if (i == currentPrime) {
//       if (m % i == 0) {
//         for(uint64_t j = i; j < m; j+=i)
//           coPrime[j] = true;
//       }
//       currentPrime = prm.next_prime(currentPrime);
//     } else if (!coPrime[i]) {
//       ret.push_back(i);
//     }
//   }
//   delete [] coPrime;
//   return ret;
// }

// uint64_t next_s(uint64_t x, uint64_t m) {
//   for(x += 1; x < m; x++) {
//     if (gcd(x, m) == 1) return x;
//   }
//   return m;
// }

// attackRet attack(
void attack(
  uint_fast16_t a, uint_fast16_t b, uint_fast16_t n,
  const char* cipherimageFilepath, const char* plainfileFilepath
) {
  auto start = chrono::high_resolution_clock::now();

  ftie ftieApp;
  // ENCRYPT
  vector<uint8_t> plainbytes = ftieApp.read_plainfile(plainfileFilepath);
  plainbytes = ftieApp.pad_bytes(plainbytes);

  // DECRYPT
  png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);
  vector<vector<vector<uint8_t>>> ciphermatrix = ftieApp.image_to_matrix(cipherimage);
  acm acmBlock(a, b, n);
  vector<vector<vector<uint8_t>>> plainmatrix = acmBlock.decrypt(ciphermatrix);

  vector<uint8_t> cipherbytes = ftieApp.matrix_to_bytes(plainmatrix);
  // now we have plainbytes and cipherbytes
  // BUILD P and Q from 2 bytes prime dataset
  vector<uint16_t> primeV;
  {
    prime prm("./data/primes_2_3mod4.bin");
    primeV = prm.to_vector();
  }
  // FIND P Q S
  uint16_t pGuess; // known prime
  uint16_t qGuess; // known prime
  uint32_t sGuess;
  bool found = false;

  uint16_t i = 0;
  while ((i < primeV.size() - 1) && !found) {
    pGuess = primeV[i];
    uint16_t j = i + 1;
    while ((j < primeV.size()) && !found) {
      qGuess = primeV[j];
      uint32_t mGuess = pGuess * qGuess;
      sGuess = 2;
      while ((sGuess < mGuess) && !found) {
        cout << pGuess << ", " << qGuess << ", " << sGuess << endl;
        if (gcd(sGuess, mGuess) != 1) {
          sGuess += 1;
          continue;
        }
        bool matched = true;
        try {
          bbs bbsBlock(pGuess, qGuess, sGuess);
          uint32_t iP = 0;
          while ((iP < plainbytes.size()) && matched) {
            uint8_t keystream = bbsBlock.next();
            uint8_t a = cipherbytes[iP * 2] - keystream;
            uint8_t b = cipherbytes[iP * 2 + 1] - 2 * keystream;
            uint8_t plainbytesGuess = a - b;
            matched = plainbytesGuess == plainbytes[iP++];
          }
        } catch (const char * msg) {
          matched = false;
        }
        if (matched) found = true;
        else sGuess += 1;
          //else sGuess = next_s(sGuess, mGuess);
      }
      j += 1;
    }
    i += 1;
  }
  auto finish = chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed = finish - start;
  if (!found) {
    pGuess = 0;
    qGuess = 0;
    sGuess = 0;
  }
  attackRet ret;
  ret.elapsed = elapsed.count();
  ret.p = pGuess;
  ret.q = qGuess;
  ret.s = sGuess;

  cout << ret.p << ", " << ret.q << ", " << ret.s << " --> " << ret.elapsed << " s"<< endl;

  // return ret;
}

string createFilePath(
  const char * prefix, uint16_t p, uint16_t q, uint32_t s, const char * postfix
) {
  stringstream ss;
  ss << prefix << "_" << p << "_" << q << "_" << s << postfix;
  return ss.str();
}

int main(int argc, char const *argv[]) {
  uint16_t p = uint16_t(stoi(argv[1]));
  uint16_t q = uint16_t(stoi(argv[2]));
  uint64_t s = uint64_t(stol(argv[3]));

  // CREATE A SAMPLE
  string plainfileFilepath = createFilePath("./dummy/plainfile", p, q, s, ".dat");
  string cipherimageFilepath = createFilePath("./dummy/cipherimage", p, q, s, ".png");

  // -- PLAINFILE
  vector<uint8_t> plainbytes(321000);
  for(int i = 0; i < plainbytes.size(); i++)
    plainbytes[i] = 170;
  ofstream outfile(plainfileFilepath.c_str(), ios::binary);
  outfile.write((const char*)&plainbytes[0], plainbytes.size());
  outfile.close();

  try {
    ftie ftieApp;
    ftieApp.encrypt(p, q, s, A, B, N, plainfileFilepath.c_str(), cipherimageFilepath.c_str());

    attack(A, B, N, cipherimageFilepath.c_str(), plainfileFilepath.c_str());
  } catch (const char * msg) {
    cout << msg << '\n';
  }
    // attackRet ret = attack(A, B, N, cipherimageFilepath, plainfileFilepath);
    // cout << ret.p << ", " << ret.q << ", " << ret.s << " --> " << ret.elapsed << " s"<< endl;


  cout << p << ", " << q << ", " << s << endl;

  return 0;
}
