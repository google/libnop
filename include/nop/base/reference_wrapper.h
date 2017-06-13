#ifndef LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_
#define LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_

#include <functional>

#include <nop/base/encoding.h>

namespace nop {

template <typename T>
struct Encoding<std::reference_wrapper<T>>
    : EncodingIO<std::reference_wrapper<T>> {
  using Type = std::reference_wrapper<T>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return Encoding<T>::Prefix(value);
  }

  static constexpr std::size_t Size(const Type& value) {
    return Encoding<T>::Size(value);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<T>::Match(prefix);
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    return Encoding<T>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    return Encoding<T>::ReadPayload(prefix, &value->get(), reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_
