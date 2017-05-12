#ifndef LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_

#include <cstdint>

namespace nop {

using EncodingType = std::uint8_t;

// First byte of an encoding specifies its type and possibly other embedded
// values.
enum class EncodingPrefix : EncodingType {
  // Positive integer type with embedded value.
  PositiveFixInt = 0x00,
  PositiveFixIntMin = 0x00,
  PositiveFixIntMax = 0x7f,
  PositiveFixIntMask = 0x7f,

  // Generic type with embedded type code.
  FixType = 0x80,
  FixTypeMin = 0x80,
  FixTypeMax = 0xbf,
  FixTypeMask = 0x3f,
  FixTypeRange = FixTypeMax - FixTypeMin + 1,

  // Nil type.
  Nil = 0xc0,

  // Generic type.
  Type = 0xc1,

  // Boolean types with embedded value.
  False = 0xc2,
  True = 0xc3,

  // Unsigned integer types.
  U8 = 0xc4,
  U16 = 0xc5,
  U32 = 0xc6,
  U64 = 0xc7,

  // Signed integer types.
  I8 = 0xc8,
  I16 = 0xc9,
  I32 = 0xca,
  I64 = 0xcb,

  // Floating point types.
  F32 = 0xcc,
  F64 = 0xcd,

  // Negative integer type with embedded value.
  NegativeFixInt = 0xe0,
  NegativeFixIntMin = 0xe0,
  NegativeFixIntMax = 0xff,
};

// General class describing the category of an encoding.
enum class EncodingClass {
  Boolean,
  Nil,
  SignedInteger,
  UnsignedInteger,
  FloatingPoint,
  Array,
  Map,
  String,
  Binary,
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_
