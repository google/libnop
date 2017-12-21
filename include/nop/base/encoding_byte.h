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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_

#include <cstdint>

namespace nop {

// First byte of an encoding specifies its type and possibly other embedded
// values.
enum class EncodingByte : std::uint8_t {
  // Positive integer type with embedded value.
  PositiveFixInt = 0x00,
  PositiveFixIntMin = 0x00,
  PositiveFixIntMax = 0x7f,
  PositiveFixIntMask = 0x7f,

  // Boolean types with embedded value.
  False = 0x00,
  True = 0x01,

  // Unsigned integer types.
  U8 = 0x80,
  U16 = 0x81,
  U32 = 0x82,
  U64 = 0x83,

  // Signed integer types.
  I8 = 0x84,
  I16 = 0x85,
  I32 = 0x86,
  I64 = 0x87,

  // Floating point types.
  F32 = 0x88,
  F64 = 0x89,

  // Reserved types.
  ReservedMin = 0x8a,
  ReservedMax = 0xb4,

  // Table types.
  Table = 0xb5,

  // Error types.
  Error = 0xb6,

  // Handle types.
  Handle = 0xb7,

  // Variant types.
  Variant = 0xb8,

  // Structure types.
  Structure = 0xb9,

  // Array types.
  Array = 0xba,

  // Map types.
  Map = 0xbb,

  // Binary types.
  Binary = 0xbc,

  // String types.
  String = 0xbd,

  // Nil type.
  Nil = 0xbe,

  // Extended type.
  Extension = 0xbf,

  // Negative integer type with embedded value.
  NegativeFixInt = 0xc0,
  NegativeFixIntMin = 0xc0,
  NegativeFixIntMax = 0xff,
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_
