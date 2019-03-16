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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_DETAIL_LOGICAL_BUFFER_H_
#define LIBNOP_INCLUDE_NOP_TYPES_DETAIL_LOGICAL_BUFFER_H_

#include <type_traits>

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
template <typename BufferType, typename SizeType, bool IsUnbounded = false,
          typename Enabled = void>
class LogicalBuffer {
  static_assert(sizeof(BufferType) != sizeof(BufferType),
                "LogicalBuffer does not have a specialization supporting the "
                "paired member types given. Make sure the correct members are "
                "grouped.");
};

// Performs static asserts to ensure that SizeType is integral and that it can
// accommodate the range required by Length.
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
template <typename BufferType, typename SizeType, bool IsUnbounded_>
class LogicalBuffer<BufferType, SizeType, IsUnbounded_,
                    EnableIfLogicalBufferPair<BufferType, SizeType>>
    : LogicalBufferStaticConstraints<BufferType, SizeType> {
 public:
  using ValueType = typename ArrayTraits<BufferType>::ElementType;
  enum : std::size_t { Length = ArrayTraits<BufferType>::Length };
  enum : bool { IsUnbounded = IsUnbounded_ };
  enum : bool {
    IsTriviallyDestructible = std::is_trivially_destructible<ValueType>::value
  };

  static_assert(
      !(Length != 1 && IsUnbounded),
      "Unbounded logical buffers must have an array element of length 1!");
  static_assert(!(IsUnbounded && !IsTriviallyDestructible),
                "Unbounded logical buffers must have trivially destructible "
                "value types!");

  constexpr LogicalBuffer(BufferType& data, SizeType& size)
      : data_{data}, size_{size} {}
  constexpr LogicalBuffer(const LogicalBuffer&) = default;
  constexpr LogicalBuffer& operator=(const LogicalBuffer&) = default;

  constexpr ValueType& operator[](std::size_t index) { return data_[index]; }
  constexpr const ValueType& operator[](std::size_t index) const {
    return data_[index];
  }

  constexpr SizeType& size() { return size_; }
  constexpr const SizeType& size() const { return size_; }

  constexpr ValueType* begin() { return &data_[0]; }
  constexpr const ValueType* begin() const { return &data_[0]; }

  constexpr ValueType* end() { return &data_[size_]; }
  constexpr const ValueType* end() const { return &data_[size_]; }

 private:
  BufferType& data_;
  SizeType& size_;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_DETAIL_LOGICAL_BUFFER_H_
