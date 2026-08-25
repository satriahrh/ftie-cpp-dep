# ftie-cp

## Compile Instruction
``g++ -o bin/ftie-cli -std=c++17 ftie/*.h ftie/*.cpp main.cpp `libpng-config --ldflags` -lgmpxx -lgmp ``

On macOS with Homebrew, Clang doesn't search `/opt/homebrew` by default and also errors on the `ftie/*.h`
glob when treated as separate translation units, so use instead:
```
g++ -o bin/ftie-cli -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib ftie/*.cpp main.cpp `libpng-config --cflags --ldflags` -lgmpxx -lgmp
```

## Requirements
- `g++ (Ubuntu 8-20180424-0ubuntu1~16.04.1) 8.0.1 20180424 (experimental) [trunk revision 259590]`
- `png++-0.2.9`
- `gmp`/`gmpxx` (arbitrary-precision arithmetic for the BBS modulus) — `brew install gmp` on macOS, or the
  `libgmp-dev` package on Linux.

## Generating BBS primes
`P`/`Q` need to be ~1024-bit primes congruent to 3 mod 4 (so the BBS modulus `P*Q` is ~2048 bits). Generate
one at a time:
```
./bin/ftie-cli genprime 1024
```
Run it twice to get distinct `P` and `Q`, then pass the printed decimal values as CLI args to `encrypt`/`decrypt`.

## System Block Diagram
![System Block Diagram](data/system-block-diagram.png)

## Deprecated System Block Diagram [[1]](https://www.researchgate.net/publication/320087595_File_To_Image_Encryption_FTIE_Menggunakan_Algoritma_Randomized_Text_Dan_Arnold_Cat_Map_ACM_Untuk_Keamanan_Transmisi_Data_Digital)
![Deprecated System Block Diagram](data/system-block-diagram-deprecated.png)
