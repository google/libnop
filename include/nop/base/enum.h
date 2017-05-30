#ifndef LIBNOP_INCLUDE_NOP_BASE_ENUM_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENUM_H_

#include <type_traits>

#include <nop/base/encoding.h>

namespace nop {

// Enable if T is an enumeration type.
template <typename T, typename ReturnType = void>
using EnableIfEnum =
    typename std::enable_if<std::is_enum<T>::value, ReturnType>::type;

//
// enum encoding format matches the encoding format of the underlying integer
// type.
//

template <typename T>
struct Encoding<T, EnableIfEnum<T>> : EncodingIO<T> {
  static constexpr EncodingByte Prefix(const T& value) {
    return Encoding<IntegerType>::Prefix(static_cast<IntegerType>(value));
  }

  static constexpr std::size_t Size(const T& value) {
    return Encoding<IntegerType>::Size(static_cast<IntegerType>(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<IntegerType>::Match(prefix);
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const T& value,
                                   Writer* writer) {
    return Encoding<IntegerType>::WritePayload(
        prefix, reinterpret_cast<const IntegerType&>(value), writer);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, T* value,
                                  Reader* reader) {
    return Encoding<IntegerType>::ReadPayload(
        prefix, reinterpret_cast<IntegerType*>(value), reader);
  }

 private:
  using IntegerType = typename std::underlying_type<T>::type;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENUM_H_
