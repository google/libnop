/*
 * Copyright 2017 The Native Object Protocols Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
