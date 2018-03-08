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

#ifndef LIBNOP_INCLUDE_NOP_BASE_TUPLE_H_
#define LIBNOP_INCLUDE_NOP_BASE_TUPLE_H_

#include <tuple>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

namespace nop {

//
// std::tuple<Ts...> encoding format:
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements must be valid encodings of each corresponding type in Ts.
//

template <typename... Types>
struct Encoding<std::tuple<Types...>> : EncodingIO<std::tuple<Types...>> {
  using Type = std::tuple<Types...>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(sizeof...(Types)) +
           Size(value, Index<sizeof...(Types)>{});
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<SizeType>::Write(sizeof...(Types), writer);
    if (!status)
      return status;
    else
      return WriteElements(value, writer, Index<sizeof...(Types)>{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != sizeof...(Types))
      return ErrorStatus::InvalidContainerLength;
    else
      return ReadElements(value, reader, Index<sizeof...(Types)>{});
  }

 private:
  template <std::size_t Index>
  using ElementType = std::remove_cv_t<
      std::remove_reference_t<std::tuple_element_t<Index, Type>>>;

  // Terminates template recursion.
  static constexpr std::size_t Size(const Type& /*value*/, Index<0>) {
    return 0;
  }

  // Recursively determines the size of all tuple elements.
  template <std::size_t index>
  static constexpr std::size_t Size(const Type& value, Index<index>) {
    return Size(value, Index<index - 1>{}) +
           Encoding<ElementType<index - 1>>::Size(std::get<index - 1>(value));
  }

  // Terminates template recursion.
  template <typename Writer>
  static constexpr Status<void> WriteElements(const Type& /*value*/, Writer* /*writer*/,
                                    Index<0>) {
    return {};
  }

  // Recursively writes tuple elements to the writer.
  template <std::size_t index, typename Writer>
  static constexpr Status<void> WriteElements(const Type& value, Writer* writer,
                                    Index<index>) {
    auto status = WriteElements(value, writer, Index<index - 1>{});
    if (!status)
      return status;

    return Encoding<ElementType<index - 1>>::Write(std::get<index - 1>(value),
                                                   writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadElements(Type* /*value*/, Reader* /*reader*/,
                                   Index<0>) {
    return {};
  }

  template <std::size_t index, typename Reader>
  static constexpr Status<void> ReadElements(Type* value, Reader* reader, Index<index>) {
    auto status = ReadElements(value, reader, Index<index - 1>{});
    if (!status)
      return status;

    return Encoding<ElementType<index - 1>>::Read(&std::get<index - 1>(*value),
                                                  reader);
  }
};

}  // namespace nop

#endif  //  LIBNOP_INCLUDE_NOP_BASE_TUPLE_H_
