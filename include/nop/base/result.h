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

#ifndef LIBNOP_INCLUDE_NOP_BASE_RESULT_H_
#define LIBNOP_INCLUDE_NOP_BASE_RESULT_H_

#include <nop/base/encoding.h>
#include <nop/base/utility.h>
#include <nop/types/result.h>

namespace nop {

//
// Result<ErrorEnum, T> encoding format when containing a value:
//
// +-------+
// | VALUE |
// +-------+
//
// VALUE must be a valid encoding of type T.
//
// Result<ErrorEnum, T> encoding format when containing an error:
//
// +-----+------+
// | ERR | ENUM |
// +-----+------+
//
// ENUM must be a valid encoding of ErrorEnum.
//

template <typename ErrorEnum, typename T>
struct Encoding<Result<ErrorEnum, T>> : EncodingIO<Result<ErrorEnum, T>> {
  using Type = Result<ErrorEnum, T>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return value.has_value() ? Encoding<T>::Prefix(value.get())
                             : EncodingByte::Error;
  }

  static constexpr std::size_t Size(const Type& value) {
    if (value.has_value()) {
      return Encoding<T>::Size(value.get());
    } else {
      return BaseEncodingSize(EncodingByte::Error) +
             Encoding<ErrorEnum>::Size(value.error());
    }
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Error || Encoding<T>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    if (value.has_value())
      return Encoding<T>::WritePayload(prefix, value.get(), writer);
    else
      return Encoding<ErrorEnum>::Write(value.error(), writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::Error) {
      ErrorEnum error_value = ErrorEnum::None;
      auto status = Encoding<ErrorEnum>::Read(&error_value, reader);
      if (!status)
        return status;

      *value = error_value;
      return {};
    } else {
      *value = T{};
      return Encoding<T>::ReadPayload(prefix, &value->get(), reader);
    }
  }
};

template <typename T>
struct IsResultType : std::false_type {};
template <typename EnumType, typename T>
struct IsResultType<Result<EnumType, T>> : std::true_type {};

// Enable if T is derived from Result<> but is not directly an instantiation of
// Result<>.
template <typename T>
using EnableIfResultType =
    typename std::enable_if<IsTemplateBaseOf<Result, T>::value &&
                            !IsResultType<T>::value>::type;

// Deduces the Result<ErrorNum, T> that the argument is derived from.
template <typename ErrorEnum, typename T>
Result<ErrorEnum, T> DeduceResultType(const Result<ErrorEnum, T>*);

// Evaluates to the Result<> that T is derived from.
template <typename T>
using ResultType = decltype(DeduceResultType(std::declval<T*>()));

// Enables the serialization of types derived from Result<>.
template <typename T>
struct Encoding<T, EnableIfResultType<T>> : Encoding<ResultType<T>> {};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_RESULT_H_
