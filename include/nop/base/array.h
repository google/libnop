#ifndef LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_
#define LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_

#include <array>
#include <numeric>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

namespace nop {

//
// std::array<T, N> encoding format for non-integral types:
//
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements are expected to be valid encodings of type T.
//
// std::array<T, N> encoding format for integral types:
//
// +-----+---------+-----//-----+
// | BIN | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements are stored as direct little-endian representation of the integral
// value; each element is sizeof(T) bytes in size.
//

template <typename T, std::size_t Length>
struct Encoding<std::array<T, Length>, EnableIfNotIntegral<T>>
    : EncodingIO<std::array<T, Length>> {
  using Type = std::array<T, Length>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(Length) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const T& element) {
                             return sum + Encoding<T>::Size(element);
                           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length, writer);
    if (!status)
      return status;

    for (const T& element : value) {
      status = Encoding<T>::Write(element, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus(EPROTO);

    for (std::uint64_t i = 0; i < Length; i++) {
      status = Encoding<T>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    return {};
  }
};

template <typename T, std::size_t Length>
struct Encoding<std::array<T, Length>, EnableIfIntegral<T>>
    : EncodingIO<std::array<T, Length>> {
  using Type = std::array<T, Length>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(Length) +
           sizeof(T) * Length;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length, writer);
    if (!status)
      return status;

    return writer->WriteRaw(value.begin(), value.end());
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus(EPROTO);

    return reader->ReadRaw(&(*value)[0], &(*value)[Length]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_
