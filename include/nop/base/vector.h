#ifndef LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_
#define LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_

#include <numeric>
#include <vector>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

namespace nop {

//
// std::vector<T> encoding format for non-integral types:
//
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements must be valid encodings of type T.
//
// std::vector<T> encoding format for integral types:
//
// +-----+---------+---//----+
// | BIN | INT64:L | L BYTES |
// +-----+---------+---//----+
//
// Where L = N * sizeof(T).
//
// Elements are stored as direct little-endian representation of the integral
// value; each element is sizeof(T) bytes in size.
//

// Specialization for non-integral types.
template <typename T, typename Allocator>
struct Encoding<std::vector<T, Allocator>, EnableIfNotIntegral<T>>
    : EncodingIO<std::vector<T, Allocator>> {
  using Type = std::vector<T, Allocator>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(value.size()) +
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
    auto status = Encoding<std::uint64_t>::Write(value.size(), writer);
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
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;

    // Clear the vector to make sure elements are inserted at the correct
    // indices. Intentionally avoid calling reserve() to prevent abuse from very
    // large size values. Regardless of the size specified in the encoding the
    // bytes remaining in the reader provide a natural upper limit to the number
    // of allocations.
    value->clear();
    for (std::uint64_t i = 0; i < size; i++) {
      T element;
      status = Encoding<T>::Read(&element, reader);
      if (!status)
        return status;

      value->push_back(std::move(element));
    }

    return {};
  }
};

// Specialization for integral types.
template <typename T, typename Allocator>
struct Encoding<std::vector<T, Allocator>, EnableIfIntegral<T>>
    : EncodingIO<std::vector<T, Allocator>> {
  using Type = std::vector<T, Allocator>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = value.size() * sizeof(T);
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(size) + size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status =
        Encoding<std::uint64_t>::Write(value.size() * sizeof(T), writer);
    if (!status)
      return status;

    return writer->WriteRaw(value.begin(), value.end());
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;

    if (size % sizeof(T) != 0)
      return ErrorStatus(EPROTO);

    const std::uint64_t length = size / sizeof(T);

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous vector sizes.
    status = reader->Ensure(length);
    if (!status)
      return status;

    value->resize(length);
    return reader->ReadRaw(&(*value)[0], &(*value)[length]);
  }
};
}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_
