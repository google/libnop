#ifndef LIBNOP_EXAMPLES_STRING_TO_HEX_H_
#define LIBNOP_EXAMPLES_STRING_TO_HEX_H_

#include <string>

namespace nop {

// Convertx the bytes of a string to a hexadecimal string.
std::string StringToHex(const std::string& input) {
  static const char* const lut = "0123456789ABCDEF";
  const size_t len = input.length();

  std::string output;
  output.reserve(2 * len);
  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = input[i];
    output.push_back(lut[c >> 4]);
    output.push_back(lut[c & 15]);
    if (i < len - 1)
      output.push_back(' ');
  }

  return output;
}

} // namespace nop

#endif // LIBNOP_EXAMPLES_STRING_TO_HEX_H_
