#ifndef LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_

#include <nop/base/encoding.h>
#include <nop/types/optional.h>

namespace nop {

//
// Optional<T> encoding formats:
//
// Empty Optional<T>:
//
// +-----+
// | NIL |
// +-----+
//
// Non-empty Optional<T>
//
// +---//----+
// | ELEMENT |
// +---//----+
//
// Element must be a valid encoding of type T.
//

template <typename T>
struct Encoding<Optional<T>> : EncodingIO<Optional<T>> {
  using Type = Optional<T>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return value ? Encoding<T>::Prefix(value.get()) : EncodingByte::Nil;
  }

  static constexpr std::size_t Size(const Type& value) {
    return value ? Encoding<T>::Size(value.get())
                 : BaseEncodingSize(EncodingByte::Nil);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Nil || Encoding<T>::Match(prefix);
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    if (value)
      return Encoding<T>::WritePayload(prefix, value.get(), writer);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::Nil) {
      value->clear();
    } else {
      T temp;
      auto status = Encoding<T>::ReadPayload(prefix, &temp, reader);
      if (!status)
        return status;

      *value = std::move(temp);
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_
