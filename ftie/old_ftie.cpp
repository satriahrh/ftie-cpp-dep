#include "old_ftie.h"


void old_ftie::encrypt(
  std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b,
  const char* plainfileFilepath, const char* cipherimageFilepath
) {
  std::vector<uint8_t> plainbytes = read_plainfile(plainfileFilepath);
  plainbytes = pad_bytes(plainbytes);

  rt rtBlock;
  std::vector<uint8_t> cipherbytesRt = rtBlock.encrypt(plainbytes, keystream);

  old_acm acmBlock;
  std::vector<uint8_t> cipherbytesAcm = acmBlock.encrypt(a, b, cipherbytesRt);

  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix = bytes_to_matrix(cipherbytesAcm);

  png::image<png::rgb_pixel> cipherimage = matrix_to_image(ciphermatrix);
  cipherimage.write(cipherimageFilepath);
}

void old_ftie::decrypt(
  std::vector<uint8_t> keystream, uint_fast16_t a, uint_fast16_t b,
  const char* cipherimageFilepath, const char* plainfileFilepath
){
  png::image<png::rgb_pixel> cipherimage(cipherimageFilepath);
  std::vector<std::vector<std::vector<uint8_t>>> ciphermatrix = image_to_matrix(cipherimage);

  std::vector<uint8_t> cipherbytesAcm = matrix_to_bytes(ciphermatrix);

  old_acm acmBlock;
  std::vector<uint8_t> cipherbytesRt = acmBlock.decrypt(a, b, cipherbytesAcm);

  rt rtBlock;
  std::vector<uint8_t> plainbytes = rtBlock.decrypt(cipherbytesRt, keystream);

  plainbytes = strip_bytes(plainbytes);
  write_plainfile(plainbytes, plainfileFilepath);
}
