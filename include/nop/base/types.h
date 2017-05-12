#ifndef LIBNOP_INCLUDE_NOP_BASE_TYPES_H_
#define LIBNOP_INCLUDE_NOP_BASE_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>

#include <nop/base/encoding.h>

namespace nop {

inline constexpr EncodingType EncodeType(bool value) {
  return value ? static_cast<EncodingType>(EncodingPrefix::True)
               : static_cast<EncodingType>(EncodingPrefix::False);
}

// Type 'char' is a little bit special in that it is distinct from 'signed char'
// and 'unsigned char'. Treating it as an unsigned 8-bit value is safe for
// encoding purposes and nicely handles 7-bit ASCII encodings as FIXINT.
inline constexpr EncodingType EncodeType(char value) {
  if (value < static_cast<char>(1 << 7))
    return value;
  else
    return static_cast<EncodingType>(EncodingPrefix::U8);
}

inline constexpr EncodingType EncodeType(std::uint8_t value) {
  if (value < (1U << 7))
    return value;
  else
    return static_cast<EncodingType>(EncodingPrefix::U8);
}
inline constexpr EncodingType EncodeType(std::int8_t value) {
  if (value >= -32)
    return value;
  else
    return static_cast<EncodingType>(EncodingPrefix::I8);
}
inline constexpr EncodingType EncodeType(std::uint16_t value) {
  if (value < (1U << 7))
    return static_cast<EncodingType>(value);
  else if (value < (1U << 8))
    return static_cast<EncodingType>(EncodingPrefix::U8);
  else
    return static_cast<EncodingType>(EncodingPrefix::U16);
}
inline constexpr EncodingType EncodeType(std::int16_t value) {
  if (value >= -32 && value <= 127)
    return static_cast<EncodingType>(value);
  else if (value >= -128 && value <= 127)
    return static_cast<EncodingType>(EncodingPrefix::I8);
  else
    return static_cast<EncodingType>(EncodingPrefix::I16);
}
inline constexpr EncodingType EncodeType(std::uint32_t value) {
  if (value < (1U << 7))
    return static_cast<EncodingType>(value);
  else if (value < (1U << 8))
    return static_cast<EncodingType>(EncodingPrefix::U8);
  else if (value < (1U << 16))
    return static_cast<EncodingType>(EncodingPrefix::U16);
  else
    return static_cast<EncodingType>(EncodingPrefix::U32);
}
inline constexpr EncodingType EncodeType(std::int32_t value) {
  if (value >= -32 && value <= 127)
    return static_cast<EncodingType>(value);
  else if (value >= -128 && value <= 127)
    return static_cast<EncodingType>(EncodingPrefix::I8);
  else if (value >= -32768 && value <= 32767)
    return static_cast<EncodingType>(EncodingPrefix::I16);
  else
    return static_cast<EncodingType>(EncodingPrefix::I32);
}
inline constexpr EncodingType EncodeType(std::uint64_t value) {
  if (value < (1ULL << 7))
    return static_cast<EncodingType>(value);
  else if (value < (1ULL << 8))
    return static_cast<EncodingType>(EncodingPrefix::U8);
  else if (value < (1ULL << 16))
    return static_cast<EncodingType>(EncodingPrefix::U16);
  else if (value < (1ULL << 32))
    return static_cast<EncodingType>(EncodingPrefix::U32);
  else
    return static_cast<EncodingType>(EncodingPrefix::U64);
}
inline constexpr EncodingType EncodeType(std::int64_t value) {
  if (value >= -32 && value <= 127)
    return static_cast<EncodingType>(value);
  else if (value >= -128 && value <= 127)  // Effectively [-128, -32).
    return static_cast<EncodingType>(EncodingPrefix::I8);
  else if (value >= -32768 && value <= 32767)
    return static_cast<EncodingType>(EncodingPrefix::I16);
  else if (value >= -2147483648 && value <= 2147483647)
    return static_cast<EncodingType>(EncodingPrefix::I32);
  else
    return static_cast<EncodingType>(EncodingPrefix::I64);
}

inline constexpr EncodingType EncodeType(float /*value*/) {
  return static_cast<EncodingType>(EncodingPrefix::F32);
}

inline constexpr EncodingType EncodeType(double /*value*/) {
  return static_cast<EncodingType>(EncodingPrefix::F64);
}

class Encoding {
 public:
  template <typename T>
  explicit Encoding(T value) : value_(EncodeType(value)) {}

  Encoding(const Encoding&) = default;
  Encoding& operator=(const Encoding&) = default;

  EncodingType value() const { return value_; }

 private:
  EncodingType value_;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_TYPES_H_
