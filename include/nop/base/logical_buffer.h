#ifndef LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_
#define LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_

#include <array>
#include <cstddef>
#include <limits>
#include <numeric>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

//
// Logical buffers support the serialization of structures that contain a pair
// of members, an array and size, that should be logically grouped together to
// behave like a sizeable buffer. This is useful in situations where supporting
// an externally-defined "C" structure with a buffer pattern is needed or where
// dynamic memory allocation is not desirable. Logical buffers are fungible with
// other array-like types, making it easy to substitute an array/size pair when
// needed.
//
// Example of defining a logical buffer in a "C" structure:
//
// // C structure defined in a public header.
// struct SomeCType {
//   char data[256];
//   size_t count;
// };
//
// To handle the externally-defined structure use the macro NOP_STRUCTURE in the
// C++ code that handles serializing the data. Parenthesis are used to group the
// pair of members to treat as a logical buffer.
//
// NOP_STRUCTURE(SomeCType, (data, count));
//
// Example of defining a C++ type with a logical buffer pair:

// template <typename T>
// struct SomeTemplateType {
//  std::array<T, 20> elements;
//  std::size_t count;
//  NOP_MEMBERS(SomeTemplateType, (elements, count));
// };
//
// Logical buffers are fungible with other array-like types:
//
// struct A {
//  int value;
//  std::vector<int> data;
//  NOP_MEMBERS(A, value, data);
// };
//
// struct B {
//  int value;
//  int data[256];
//  size_t count;
//  NOP_MEMBERS(B, value, (data, count));
// };
//
// static_assert(nop::IsFungible<A, B>::value, "!!");
//

namespace nop {

// Trait that determines whether the given types BufferType and SizeType
// constitute a valid logical buffer pair.
template <typename BufferType, typename SizeType>
struct IsLogicalBufferPair : std::false_type {};
template <typename T, typename SizeType, std::size_t Length>
struct IsLogicalBufferPair<T[Length], SizeType> : std::true_type {};
template <typename T, typename SizeType, std::size_t Length>
struct IsLogicalBufferPair<std::array<T, Length>, SizeType> : std::true_type {};
template <typename T, typename SizeType, std::size_t Length>
struct IsLogicalBufferPair<const std::array<T, Length>, SizeType>
    : std::true_type {};

// Enable if BufferType and SizeType constitute a valid logical buffer pair.
template <typename BufferType, typename SizeType>
using EnableIfLogicalBufferPair = typename std::enable_if<
    IsLogicalBufferPair<BufferType, SizeType>::value>::type;

// LogicalBuffer captures two members of a class or structure that taken
// together represent a logical buffer. The first member is an array type that
// holds the data for the buffer and the second member is an integral type that
// stores the number of elements in use.
template <typename BufferType, typename SizeType, typename Enabled = void>
class LogicalBuffer {
  static_assert(sizeof(BufferType) != sizeof(BufferType),
                "LogicalBuffer does not have a specialization supporting the "
                "paired member types given. Make sure the correct members are "
                "grouped.");
};

// Performs static asserts to ensure that SizeType is integral and that it can
// accomodate the range required by Length.
template <typename BufferType, typename SizeType>
struct LogicalBufferStaticConstraints {
  static_assert(
      std::is_integral<SizeType>::value,
      "The size member of a logical buffer pair must be an integral type.");
  static_assert(ArrayTraits<BufferType>::Length <=
                    std::numeric_limits<SizeType>::max(),
                "The size member of a logical buffer pair must have sufficient "
                "range to count the elements of the array member.");
};

// Captures references to the array and size members of a user-defined structure
// that are grouped into a logical buffer type.
template <typename BufferType, typename SizeType>
class LogicalBuffer<BufferType, SizeType,
                    EnableIfLogicalBufferPair<BufferType, SizeType>>
    : LogicalBufferStaticConstraints<BufferType, SizeType> {
 public:
  using ValueType = typename ArrayTraits<BufferType>::ElementType;

  LogicalBuffer(BufferType& data, SizeType& size) : data_{data}, size_{size} {}
  LogicalBuffer(const LogicalBuffer&) = default;
  LogicalBuffer& operator=(const LogicalBuffer&) = default;

  ValueType& operator[](std::size_t index) { return data_[index]; }
  const ValueType& operator[](std::size_t index) const { return data_[index]; }

  SizeType& size() { return size_; }
  const SizeType& size() const { return size_; }

  ValueType* begin() { return &data_[0]; }
  const ValueType* begin() const { return &data_[0]; }

  ValueType* end() { return &data_[size_]; }
  const ValueType* end() const { return &data_[size_]; }

 private:
  BufferType& data_;
  SizeType& size_;
};

// Encoding type that handles non-integral element types. Logical buffers of
// non-integral element types are encoded the same as non-integral arrays using
// the ARRAY encoding.
template <typename BufferType, typename SizeType>
struct Encoding<
    LogicalBuffer<BufferType, SizeType>,
    EnableIfNotIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType>> {
  using Type = LogicalBuffer<BufferType, SizeType>;
  using ValueType =
      std::remove_const_t<typename ArrayTraits<BufferType>::ElementType>;
  enum : std::size_t { Length = ArrayTraits<BufferType>::Length };

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(value.size()) +
           std::accumulate(
               std::begin(value), std::end(value), 0U,
               [](const std::size_t& sum, const ValueType& element) {
                 return sum + Encoding<ValueType>::Size(element);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    const std::uint64_t size = static_cast<std::uint64_t>(value.size());
    if (size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status = Encoding<std::uint64_t>::Write(size, writer);
    if (!status)
      return status;

    for (std::uint64_t i = 0; i < size; i++) {
      status = Encoding<ValueType>::Write(value[i], writer);
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
    else if (size > Length)
      return ErrorStatus::InvalidContainerLength;

    for (std::uint64_t i = 0; i < size; i++) {
      status = Encoding<ValueType>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    value->size() = size;
    return {};
  }
};

// Encoding type that handles integral element types. Logical buffers of
// integral element types are encoded the same as arrays with integral elements
// using the BINARY encoding.
template <typename BufferType, typename SizeType>
struct Encoding<LogicalBuffer<BufferType, SizeType>,
                EnableIfIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType>> {
  using Type = LogicalBuffer<BufferType, SizeType>;
  using ValueType =
      std::remove_const_t<typename ArrayTraits<BufferType>::ElementType>;
  enum : std::size_t { Length = ArrayTraits<BufferType>::Length };

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = value.size() * sizeof(ValueType);
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(size) + size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    const std::uint64_t size = value.size();
    if (size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status =
        Encoding<std::uint64_t>::Write(size * sizeof(ValueType), writer);
    if (!status)
      return status;

    return writer->WriteRaw(value.begin(), value.end());
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    std::uint64_t size_bytes;
    auto status = Encoding<std::uint64_t>::Read(&size_bytes, reader);
    if (!status) {
      return status;
    } else if (size_bytes > Length * sizeof(ValueType) ||
               size_bytes % sizeof(ValueType) != 0) {
      return ErrorStatus::InvalidContainerLength;
    }

    const std::uint64_t size = size_bytes / sizeof(ValueType);
    value->size() = size;
    return reader->ReadRaw(value->begin(), value->end());
  }
};

}  // namespace nop

#endif  //  LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_
