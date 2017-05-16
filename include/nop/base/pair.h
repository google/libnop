#ifndef LIBNOP_INCLUDE_NOP_BASE_PAIR_H_
#define LIBNOP_INCLUDE_NOP_BASE_PAIR_H_

#include <utility>

#include <nop/base/encoding.h>

namespace nop {

template <typename T, typename U>
struct Encoding<std::pair<T, U>> : EncodingIO<std::pair<T, U>> {
  using Type = std::pair<T, U>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) + Encoding<std::uint64_t>::Size(2u) +
           Encoding<T>::Size(value.first) + Encoding<U>::Size(value.second);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(2u, writer);
    if (!status)
      return status;

    status = Encoding<T>::Write(value.first, writer);
    if (!status)
      return status;

    return Encoding<U>::Write(value.second, writer);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != 2u)
      return ErrorStatus(EIO);

    status = Encoding<T>::Read(&value->first, reader);
    if (!status)
      return status;

    return Encoding<U>::Read(&value->second, reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_PAIR_H_
