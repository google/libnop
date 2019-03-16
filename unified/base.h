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

#ifndef LIBNOP_INCLUDE_NOP_PROTOCOL_H_
#define LIBNOP_INCLUDE_NOP_PROTOCOL_H_

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

#ifndef LIBNOP_INCLUDE_NOP_STATUS_H_
#define LIBNOP_INCLUDE_NOP_STATUS_H_

#include <string>

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

#ifndef LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_
#define LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_

#include <type_traits>

namespace nop {

// Result is a template type that contains either an error value of type
// ErrorEnum or a value of type T. Functions can use Result as a return type to
// unambiguously signal success with a value or failure with an error code.
//
// Result imposes these rules on the types it is instantiated with:
//   1. EnumType must be an enum class and must define a constant named "None",
//      such that ErrorEnum::None is a valid constant expression.
//   2. Type T must not be the same type as ErrorEnum.
//
// Result uses union semantics and lazy initialization to increase efficiency
// and convenience, particularly where T is not default constructible.
//
// Users of Result may find it convenient to subclass the type in the following
// way to simplify type expressions that use a common set of error codes:
//
// namespace MyNamespace {
//
// enum class Error {
//   None, // Required by Result.
//   InvalidArgument,
//   TimedOut,
//   PermissionDenied,
// };
//
// template <typename T>
// struct Result : nop::Result<Error, T> {
//   using nop::Result<Error, T>::Result;
//   std::string GetErrorMessage() const {
//     switch (this->error()) {
//       case Error::None: return "No Error";
//       case Error::InvalidArgument: return "Invalid Argument";
//       case Error::TimedOut: return "Timed Out";
//       case Error::PermissionDenied: return "Permission Denied";
//       default: return "Unknown Error";
//     }
//   }
// };
//
// }  // namespace MyNamespace
//
template <typename ErrorEnum, typename T>
class Result {
  static_assert(std::is_enum<ErrorEnum>::value,
                "ErrorEnum must be an enum class.");
  static_assert(!std::is_same<std::decay_t<ErrorEnum>, std::decay_t<T>>::value,
                "ErrorEnum and T must not be the same type.");

 public:
  constexpr Result() : error_{ErrorEnum::None}, state_{State::Empty} {}
  constexpr Result(const T& value) : value_{value}, state_{State::Value} {}
  constexpr Result(T&& value)
      : value_{std::move(value)}, state_{State::Value} {}
  constexpr Result(const Result& other)
      : error_{ErrorEnum::None}, state_{State::Empty} {
    *this = other;
  }
  constexpr Result(Result&& other)
      : error_{ErrorEnum::None}, state_{State::Empty} {
    *this = std::move(other);
  }
  constexpr Result(ErrorEnum error)
      : error_{error},
        state_{error == ErrorEnum::None ? State::Empty : State::Error} {}

  ~Result() { Destruct(); }

  constexpr Result& operator=(const Result& other) {
    if (this != &other) {
      if (other.has_value())
        Assign(other.value_);
      else
        Assign(other.error_);
    }
    return *this;
  }
  constexpr Result& operator=(Result&& other) {
    if (this != &other) {
      if (other.has_value())
        Assign(std::move(other.value_));
      else
        Assign(other.error_);

      other.Destruct();
    }
    return *this;
  }

  constexpr Result& operator=(const T& value) {
    Assign(value);
    return *this;
  }
  constexpr Result& operator=(T&& value) {
    Assign(std::move(value));
    return *this;
  }
  constexpr Result& operator=(ErrorEnum error) {
    Assign(error);
    return *this;
  }

  constexpr bool has_value() const { return state_ == State::Value; }
  constexpr bool has_error() const { return state_ == State::Error; }

  constexpr explicit operator bool() const { return has_value(); }

  constexpr ErrorEnum error() const {
    if (has_error())
      return error_;
    else
      return ErrorEnum::None;
  }

  constexpr const T& get() const { return *value(); }
  constexpr T& get() { return *value(); }
  constexpr T&& take() { return std::move(*value()); }

  constexpr void clear() { Destruct(); }

 private:
  constexpr void Assign(const T& value) {
    if (has_value()) {
      value_ = value;
    } else {
      new (&value_) T(value);
      state_ = State::Value;
    }
  }

  constexpr void Assign(T&& value) {
    if (has_value()) {
      value_ = std::move(value);
    } else {
      new (&value_) T(std::move(value));
      state_ = State::Value;
    }
  }

  constexpr void Assign(ErrorEnum error) {
    Destruct();
    if (error != ErrorEnum::None) {
      error_ = error;
      state_ = State::Error;
    }
  }

  constexpr void Destruct() {
    if (has_value())
      value_.~T();
    error_ = ErrorEnum::None;
    state_ = State::Empty;
  }

  constexpr const T* value() const { return has_value() ? &value_ : nullptr; }
  constexpr T* value() { return has_value() ? &value_ : nullptr; }

  enum class State {
    Empty,
    Error,
    Value,
  };

  union {
    ErrorEnum error_;
    T value_;
  };
  State state_;
};

// Specialization of Result for void types.
template <typename ErrorEnum>
class Result<ErrorEnum, void> {
  static_assert(std::is_enum<ErrorEnum>::value,
                "ErrorEnum must be an enum class.");

 public:
  constexpr Result() : error_{ErrorEnum::None} {}
  constexpr Result(ErrorEnum error) : error_{error} {}
  constexpr Result(const Result& other) : error_{other.error_} {}
  constexpr Result(Result&& other) : error_{other.error_} { other.clear(); }

  ~Result() = default;

  constexpr Result& operator=(const Result& other) {
    if (this != &other) {
      error_ = other.error_;
    }
    return *this;
  }
  constexpr Result& operator=(Result&& other) {
    if (this != &other) {
      error_ = other.error_;
      other.clear();
    }
    return *this;
  }

  constexpr bool has_error() const { return error_ != ErrorEnum::None; }
  constexpr explicit operator bool() const { return !has_error(); }

  constexpr ErrorEnum error() const { return error_; }

  constexpr void clear() { error_ = ErrorEnum::None; }

 private:
  ErrorEnum error_;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_


//
// Status<T> is the return type used by the serialization engine to return
// either success and a value or an error indicating the nature of the failure.
// This type is based on the more general Result<ErrorEnum, T>.
//

namespace nop {

enum class ErrorStatus {
  None,                    // 0
  UnexpectedEncodingType,  // 1
  UnexpectedHandleType,    // 2
  UnexpectedVariantType,   // 3
  InvalidContainerLength,  // 4
  InvalidMemberCount,      // 5
  InvalidStringLength,     // 6
  InvalidTableHash,        // 7
  InvalidHandleReference,  // 8
  InvalidHandleValue,      // 9
  InvalidInterfaceMethod,  // 10
  DuplicateTableEntry,     // 11
  ReadLimitReached,        // 12
  WriteLimitReached,       // 13
  StreamError,             // 14
  ProtocolError,           // 15
  IOError,                 // 16
  SystemError,             // 17
  DebugError,              // 18
};

template <typename T>
struct Status : Result<ErrorStatus, T> {
  using Result<ErrorStatus, T>::Result;

  const char* GetErrorMessage() const {
    switch (this->error()) {
      case ErrorStatus::None:
        return "No Error";
      case ErrorStatus::UnexpectedEncodingType:
        return "Unexpected Encoding Type";
      case ErrorStatus::UnexpectedHandleType:
        return "Unexpected Handle Type";
      case ErrorStatus::UnexpectedVariantType:
        return "Unexpected Variant Type";
      case ErrorStatus::InvalidContainerLength:
        return "Invalid Container Length";
      case ErrorStatus::InvalidMemberCount:
        return "Invalid Member Count";
      case ErrorStatus::InvalidStringLength:
        return "Invalid String Length";
      case ErrorStatus::InvalidTableHash:
        return "Invalid Table Hash";
      case ErrorStatus::InvalidHandleReference:
        return "Invalid Handle Reference";
      case ErrorStatus::InvalidHandleValue:
        return "Invalid Handle Value";
      case ErrorStatus::InvalidInterfaceMethod:
        return "Invalid Interface Method";
      case ErrorStatus::DuplicateTableEntry:
        return "Duplicate Table Hash";
      case ErrorStatus::ReadLimitReached:
        return "Read Limit Reached";
      case ErrorStatus::WriteLimitReached:
        return "Write Limit Reached";
      case ErrorStatus::StreamError:
        return "Stream Error";
      case ErrorStatus::ProtocolError:
        return "Protocol Error";
      case ErrorStatus::IOError:
        return "IO Error";
      case ErrorStatus::SystemError:
        return "System Error";
      case ErrorStatus::DebugError:
        return "Debug Error";
      default:
        return "Unknown Error";
    }
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_STATUS_H_

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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_

#include <array>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_
#define LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_

#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_

#include <errno.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_

#include <cstdint>

namespace nop {

// First byte of an encoding specifies its type and possibly other embedded
// values.
enum class EncodingByte : std::uint8_t {
  // Positive integer type with embedded value.
  PositiveFixInt = 0x00,
  PositiveFixIntMin = 0x00,
  PositiveFixIntMax = 0x7f,
  PositiveFixIntMask = 0x7f,

  // Boolean types with embedded value.
  False = 0x00,
  True = 0x01,

  // Unsigned integer types.
  U8 = 0x80,
  U16 = 0x81,
  U32 = 0x82,
  U64 = 0x83,

  // Signed integer types.
  I8 = 0x84,
  I16 = 0x85,
  I32 = 0x86,
  I64 = 0x87,

  // Floating point types.
  F32 = 0x88,
  F64 = 0x89,

  // Reserved types.
  ReservedMin = 0x8a,
  ReservedMax = 0xb4,

  // Table types.
  Table = 0xb5,

  // Error types.
  Error = 0xb6,

  // Handle types.
  Handle = 0xb7,

  // Variant types.
  Variant = 0xb8,

  // Structure types.
  Structure = 0xb9,

  // Array types.
  Array = 0xba,

  // Map types.
  Map = 0xbb,

  // Binary types.
  Binary = 0xbc,

  // String types.
  String = 0xbd,

  // Nil type.
  Nil = 0xbe,

  // Extended type.
  Extension = 0xbf,

  // Negative integer type with embedded value.
  NegativeFixInt = 0xc0,
  NegativeFixIntMin = 0xc0,
  NegativeFixIntMax = 0xff,
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENCODING_BYTE_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
#define LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_

#include <cstddef>
#include <type_traits>

/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_

#include <type_traits>

namespace nop {

namespace detail {

// Utility to deduce the template type from a derived type.
template <template <typename...> class TT, typename... Ts>
std::true_type DeduceTemplateType(const TT<Ts...>*);
template <template <typename...> class TT>
std::false_type DeduceTemplateType(...);

}  // namespace detail

// Utility determining whether template type TT<...> is a base of type T.
template <template <typename...> class TT, typename T>
using IsTemplateBaseOf =
    decltype(detail::DeduceTemplateType<TT>(std::declval<T*>()));

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_

/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_

namespace nop {

namespace detail {

template <typename... Ts>
struct MakeVoid {
  using Type = void;
};

}  // namespace detail

// Utility type for SFINAE expression evaluation. C++14 equivalent to C++17
// std::void_t.
//
// This approach addresses the issue with older C++14 compilers described in:
// http://open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#1558
template <typename... Ts>
using Void = typename detail::MakeVoid<Ts...>::Type;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_


namespace nop {

// Counting template for recursive template definitions.
template <std::size_t>
struct Index {};

// Passthrough type.
template <typename T>
using Identity = T;

// Trait to determine if all the types in a parameter pack are integral types.
template <typename...>
struct IsIntegral;
template <typename T>
struct IsIntegral<T> : std::is_integral<T> {};
template <typename First, typename... Rest>
struct IsIntegral<First, Rest...>
    : std::integral_constant<bool, IsIntegral<First>::value &&
                                       IsIntegral<Rest...>::value> {};

// Trait to determine if all types in a parameter pack are arithmetic types.
template <typename...>
struct IsArithmetic;
template <typename T>
struct IsArithmetic<T> : std::is_arithmetic<T> {};
template <typename First, typename... Rest>
struct IsArithmetic<First, Rest...>
    : std::integral_constant<bool, IsArithmetic<First>::value &&
                                       IsArithmetic<Rest...>::value> {};

// Enable if every entry of Types is an integral type.
template <typename... Types>
using EnableIfIntegral =
    typename std::enable_if<IsIntegral<Types...>::value>::type;

// Enable if any entry of Types is not an integral type.
template <typename... Types>
using EnableIfNotIntegral =
    typename std::enable_if<!IsIntegral<Types...>::value>::type;

// Enable if every entry of Types is an arithmetic type.
template <typename... Types>
using EnableIfArithmetic =
    typename std::enable_if<IsArithmetic<Types...>::value>::type;

// Enable if every entry of Types is not an arithmetic type.
template <typename... Types>
using EnableIfNotArithmetic =
    typename std::enable_if<!IsArithmetic<Types...>::value>::type;

template <typename T, typename U>
using EnableIfConvertible =
    typename std::enable_if<std::is_convertible<T, U>::value>::type;

// Utility type to retrieve the first type in a parameter pack.
template <typename...>
struct FirstType {};
template <typename First, typename... Rest>
struct FirstType<First, Rest...> {
  using Type = First;
};
template <typename... Ts>
using First = typename FirstType<Ts...>::Type;

// Determines the value type and extent of C/C++ array types.
template <typename T>
struct ArrayTraits : std::false_type {};

template <typename T, std::size_t Length_>
struct ArrayTraits<T[Length_]> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = T[Length];
};
template <typename T, std::size_t Length_>
struct ArrayTraits<std::array<T, Length_>> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = std::array<T, Length>;
};
template <typename T, std::size_t Length_>
struct ArrayTraits<const std::array<T, Length_>> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = const std::array<T, Length>;
};

// Enable if T is an array type.
template <typename T, typename Return = void>
using EnableIfArray =
    typename std::enable_if<ArrayTraits<T>::value, Return>::type;

// Logical AND over template parameter pack.
template <typename... T>
struct And : std::true_type {};
template <typename A>
struct And<A> : A {};
template <typename A, typename B>
struct And<A, B> : std::integral_constant<bool, A::value && B::value> {};
template <typename A, typename B, typename... Rest>
struct And<A, B, Rest...> : And<A, And<B, Rest...>> {};

// Logical OR over template parameter pack.
template <typename... T>
struct Or : std::true_type {};
template <typename A>
struct Or<A> : A {};
template <typename A, typename B>
struct Or<A, B> : std::integral_constant<bool, A::value || B::value> {};
template <typename A, typename B, typename... Rest>
struct Or<A, B, Rest...> : Or<A, Or<B, Rest...>> {};

// Utility to determine whether a set of one or more types is a true set,
// containing no duplicates, according to the given comparison template. The
// comparison template must accept two type arguments and evaluate to true if
// its arguments are the same according to the intended criteria.
//
// An example using integer types and std::is_same for comparison:
//
//  static_assert(IsUnique<std::is_same, int, short, int>::value,
//                "Types in set are not unique!");
//
template <template <typename, typename> class, typename...>
struct IsUnique;
template <template <typename, typename> class Same>
struct IsUnique<Same> : std::true_type {};
template <template <typename, typename> class Same, typename T>
struct IsUnique<Same, T> : std::true_type {};
template <template <typename, typename> class Same, typename First,
          typename Second>
struct IsUnique<Same, First, Second>
    : std::integral_constant<bool, !Same<First, Second>::value> {};
template <template <typename, typename> class Same, typename First,
          typename Second, typename... Rest>
struct IsUnique<Same, First, Second, Rest...>
    : And<IsUnique<Same, First, Second>, IsUnique<Same, First, Rest...>,
          IsUnique<Same, Second, Rest...>> {};

// Utility to determine whether all types in a set compare the same using the
// given comparison.
//
// An example using integer types and std::is_same for comparison:
//
//  static_assert(IsSame<std::is_same, int, short, int>::value,
//                "Types in set are not the same!");
//
template <template <typename, typename> class, typename...>
struct IsSame;
template <template <typename, typename> class Same>
struct IsSame<Same> : std::true_type {};
template <template <typename, typename> class Same, typename T>
struct IsSame<Same, T> : std::true_type {};
template <template <typename, typename> class Same, typename First,
          typename... Rest>
struct IsSame<Same, First, Rest...> : And<Same<First, Rest>...> {};

// Utility types for SFINAE expression matching either regular or template
// types.
template <typename T, typename U>
std::is_same<T, U> MatchType();

template <template <typename...> class TT, template <typename...> class UU,
          typename... Ts>
std::is_same<TT<Ts...>, UU<Ts...>> MatchTemplate();

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_



namespace nop {

// Defines the size type for container and other formats that have a
// count/length field. This is defined to be std::uint64_t on 64bit or greater
// platforms, std::uint32_t on 32bit or smaller platforms.
using SizeType =
    std::conditional_t<sizeof(std::size_t) >= sizeof(std::uint64_t),
                       std::uint64_t, std::uint32_t>;

// Returns the size of the base encodings excluding extension payloads.
inline constexpr std::size_t BaseEncodingSize(EncodingByte prefix) {
  if ((prefix >= EncodingByte::PositiveFixIntMin &&
       prefix <= EncodingByte::PositiveFixIntMax) ||
      (prefix >= EncodingByte::NegativeFixIntMin &&
       prefix <= EncodingByte::NegativeFixIntMax)) {
    return 1U;
  }

  switch (prefix) {
    /* case EncodingByte::False ... EncodingByte::True: */
    case EncodingByte::Table:
    case EncodingByte::Error:
    case EncodingByte::Handle:
    case EncodingByte::Variant:
    case EncodingByte::Structure:
    case EncodingByte::Array:
    case EncodingByte::Map:
    case EncodingByte::Binary:
    case EncodingByte::String:
    case EncodingByte::Nil:
    case EncodingByte::Extension:
      return 1U;

    case EncodingByte::U8:
    case EncodingByte::I8:
      return 2U;

    case EncodingByte::U16:
    case EncodingByte::I16:
      return 3U;

    case EncodingByte::U32:
    case EncodingByte::I32:
    case EncodingByte::F32:
      return 5U;

    case EncodingByte::U64:
    case EncodingByte::I64:
    case EncodingByte::F64:
      return 9U;

    /* case EncodingByte::ReservedMin ... EncodingByte::ReservedMax: */
    default:
      return 0U;
  }
}

// Base type for all encoding templates. If type T does not have a
// specialization this template generates a static assert.
template <typename T, typename Enabled = void>
struct Encoding {
  // Generate a clear compiler error if there is no encoding specified for a
  // type passed to the serialization engine.
  static_assert(sizeof(T) != sizeof(T),
                "Encoding<T> must be specilaized for type T. Make sure to "
                "include the appropriate encoder header.");
};

// Implements general IO for encoding types. May also be mixed-in with an
// Encoding<T> specialization to provide uniform access to Read/Write through
// the specilization itself.
// Example:
//    template <>
//    struct Encoding<SomeType> : EncodingIO<SomeType> { ... };
//
//    ...
//
//    Encoding<SomeType>::Write(...);
//
template <typename T>
struct EncodingIO {
  template <typename Writer>
  static constexpr Status<void> Write(const T& value, Writer* writer) {
    EncodingByte prefix = Encoding<T>::Prefix(value);
    auto status = writer->Write(static_cast<std::uint8_t>(prefix));
    if (!status)
      return status;
    else
      return Encoding<T>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> Read(T* value, Reader* reader) {
    std::uint8_t prefix_byte = 0;
    auto status = reader->Read(&prefix_byte);
    if (!status)
      return status;

    const EncodingByte prefix = static_cast<EncodingByte>(prefix_byte);
    if (Encoding<T>::Match(prefix))
      return Encoding<T>::ReadPayload(prefix, value, reader);
    else
      return ErrorStatus::UnexpectedEncodingType;
  }

 protected:
  template <typename As, typename From, typename Writer,
            typename Enabled = EnableIfArithmetic<As, From>>
  static constexpr Status<void> WriteAs(From value, Writer* writer) {
    As temp = static_cast<As>(value);
    return writer->Write(&temp, &temp + 1);
  }

  template <typename As, typename From, typename Reader,
            typename Enabled = EnableIfArithmetic<As, From>>
  static constexpr Status<void> ReadAs(From* value, Reader* reader) {
    As temp = 0;
    auto status = reader->Read(&temp, &temp + 1);
    if (!status)
      return status;

    *value = static_cast<From>(temp);
    return {};
  }
};

// Forwards reference types to the underlying type encoder.
template <typename T>
struct Encoding<T&&> : Encoding<T> {};
template <typename T>
struct Encoding<const T&> : Encoding<T> {};

//
// Encodings for atrithmetic types. Most encodings depend on these for size and
// type ids. These encodings assume two's complement hardware. This might change
// if there is a good use case to support one's complement hardware.
//

// Assert that the hardware uses two's complement for signed integers.
static_assert(-1 == ~0,
              "One's complement hardware is not currently supported!");

//
// bool encoding formats:
//
// +-------+
// | FALSE |
// +-------+
//
// +------+
// | TRUE |
// +------+
//

template <>
struct Encoding<bool> : EncodingIO<bool> {
  using Type = bool;

  static constexpr EncodingByte Prefix(bool value) {
    return value ? EncodingByte::True : EncodingByte::False;
  }

  static constexpr std::size_t Size(bool value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::True || prefix == EncodingByte::False;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             bool /*value*/,
                                             Writer* /*writer*/) {
    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, bool* value,
                                            Reader* /*reader*/) {
    *value = static_cast<bool>(prefix);
    return {};
  }
};

//
// char encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// Type 'char' is a little bit special in that it is distinct from 'signed char'
// and 'unsigned char'. Treating it as an unsigned 8-bit value is safe for
// encoding purposes and nicely handles 7-bit ASCII encodings as POSFIXINT.
//

template <>
struct Encoding<char> : EncodingIO<char> {
  static constexpr EncodingByte Prefix(char value) {
    if (static_cast<std::uint8_t>(value) < (1U << 7))
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::U8;
  }

  static constexpr std::size_t Size(char value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           prefix == EncodingByte::U8;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix, char value,
                                             Writer* writer) {
    if (prefix == EncodingByte::U8)
      return WriteAs<std::uint8_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, char* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return ReadAs<std::uint8_t>(value, reader);
    } else {
      *value = static_cast<char>(prefix);
      return {};
    }
  }
};

//
// std::uint8_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//

template <>
struct Encoding<std::uint8_t> : EncodingIO<std::uint8_t> {
  static constexpr EncodingByte Prefix(std::uint8_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::U8;
  }

  static constexpr std::size_t Size(std::uint8_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           prefix == EncodingByte::U8;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::uint8_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::U8)
      return WriteAs<std::uint8_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::uint8_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return ReadAs<std::uint8_t>(value, reader);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int8_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//

template <>
struct Encoding<std::int8_t> : EncodingIO<std::int8_t> {
  static constexpr EncodingByte Prefix(std::int8_t value) {
    if (value >= -64)
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::I8;
  }

  static constexpr std::size_t Size(std::int8_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           (prefix >= EncodingByte::NegativeFixIntMin &&
            prefix <= EncodingByte::NegativeFixIntMax) ||
           prefix == EncodingByte::I8;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::int8_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::I8)
      return WriteAs<std::int8_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::int8_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return ReadAs<std::int8_t>(value, reader);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint16_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint16_t> : EncodingIO<std::uint16_t> {
  static constexpr EncodingByte Prefix(std::uint16_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1U << 8))
      return EncodingByte::U8;
    else
      return EncodingByte::U16;
  }

  static constexpr std::size_t Size(std::uint16_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint8_t>::Match(prefix) || prefix == EncodingByte::U16;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::uint16_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::U8)
      return WriteAs<std::uint8_t>(value, writer);
    else if (prefix == EncodingByte::U16)
      return WriteAs<std::uint16_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::uint16_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return ReadAs<std::uint8_t>(value, reader);
    } else if (prefix == EncodingByte::U16) {
      return ReadAs<std::uint16_t>(value, reader);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int16_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int16_t> : EncodingIO<std::int16_t> {
  static constexpr EncodingByte Prefix(std::int16_t value) {
    if (value >= -64 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)
      return EncodingByte::I8;
    else
      return EncodingByte::I16;
  }

  static constexpr std::size_t Size(std::int16_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int8_t>::Match(prefix) || prefix == EncodingByte::I16;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::int16_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::I8)
      return WriteAs<std::int8_t>(value, writer);
    else if (prefix == EncodingByte::I16)
      return WriteAs<std::int16_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::int16_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return ReadAs<std::int8_t>(value, reader);
    } else if (prefix == EncodingByte::I16) {
      return ReadAs<std::int16_t>(value, reader);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint32_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U32 | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint32_t> : EncodingIO<std::uint32_t> {
  static constexpr EncodingByte Prefix(std::uint32_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1U << 8))
      return EncodingByte::U8;
    else if (value < (1U << 16))
      return EncodingByte::U16;
    else
      return EncodingByte::U32;
  }

  static constexpr std::size_t Size(std::uint32_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint16_t>::Match(prefix) ||
           prefix == EncodingByte::U32;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::uint32_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::U8)
      return WriteAs<std::uint8_t>(value, writer);
    else if (prefix == EncodingByte::U16)
      return WriteAs<std::uint16_t>(value, writer);
    else if (prefix == EncodingByte::U32)
      return WriteAs<std::uint32_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::uint32_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return ReadAs<std::uint8_t>(value, reader);
    } else if (prefix == EncodingByte::U16) {
      return ReadAs<std::uint16_t>(value, reader);
    } else if (prefix == EncodingByte::U32) {
      return ReadAs<std::uint32_t>(value, reader);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int32_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I32 | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int32_t> : EncodingIO<std::int32_t> {
  static constexpr EncodingByte Prefix(std::int32_t value) {
    if (value >= -64 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)
      return EncodingByte::I8;
    else if (value >= -32768 && value <= 32767)
      return EncodingByte::I16;
    else
      return EncodingByte::I32;
  }

  static constexpr std::size_t Size(std::int32_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int16_t>::Match(prefix) || prefix == EncodingByte::I32;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::int32_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::I8)
      return WriteAs<std::int8_t>(value, writer);
    else if (prefix == EncodingByte::I16)
      return WriteAs<std::int16_t>(value, writer);
    else if (prefix == EncodingByte::I32)
      return WriteAs<std::int32_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::int32_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return ReadAs<std::int8_t>(value, reader);
    } else if (prefix == EncodingByte::I16) {
      return ReadAs<std::int16_t>(value, reader);
    } else if (prefix == EncodingByte::I32) {
      return ReadAs<std::int32_t>(value, reader);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint64_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U32 | 4 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U64 | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint64_t> : EncodingIO<std::uint64_t> {
  static constexpr EncodingByte Prefix(std::uint64_t value) {
    if (value < (1ULL << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1ULL << 8))
      return EncodingByte::U8;
    else if (value < (1ULL << 16))
      return EncodingByte::U16;
    else if (value < (1ULL << 32))
      return EncodingByte::U32;
    else
      return EncodingByte::U64;
  }

  static constexpr std::size_t Size(std::uint64_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint32_t>::Match(prefix) ||
           prefix == EncodingByte::U64;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::uint64_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::U8)
      return WriteAs<std::uint8_t>(value, writer);
    else if (prefix == EncodingByte::U16)
      return WriteAs<std::uint16_t>(value, writer);
    else if (prefix == EncodingByte::U32)
      return WriteAs<std::uint32_t>(value, writer);
    else if (prefix == EncodingByte::U64)
      return WriteAs<std::uint64_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::uint64_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return ReadAs<std::uint8_t>(value, reader);
    } else if (prefix == EncodingByte::U16) {
      return ReadAs<std::uint16_t>(value, reader);
    } else if (prefix == EncodingByte::U32) {
      return ReadAs<std::uint32_t>(value, reader);
    } else if (prefix == EncodingByte::U64) {
      return ReadAs<std::uint64_t>(value, reader);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int64_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I32 | 4 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I64 | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int64_t> : EncodingIO<std::int64_t> {
  static constexpr EncodingByte Prefix(std::int64_t value) {
    if (value >= -64 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)  // Effectively [-128, -64).
      return EncodingByte::I8;
    else if (value >= -32768 && value <= 32767)
      return EncodingByte::I16;
    else if (value >= -2147483648 && value <= 2147483647)
      return EncodingByte::I32;
    else
      return EncodingByte::I64;
  }

  static constexpr std::size_t Size(std::int64_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int32_t>::Match(prefix) || prefix == EncodingByte::I64;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::int64_t value,
                                             Writer* writer) {
    if (prefix == EncodingByte::I8)
      return WriteAs<std::int8_t>(value, writer);
    else if (prefix == EncodingByte::I16)
      return WriteAs<std::int16_t>(value, writer);
    else if (prefix == EncodingByte::I32)
      return WriteAs<std::int32_t>(value, writer);
    else if (prefix == EncodingByte::I64)
      return WriteAs<std::int64_t>(value, writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::int64_t* value,
                                            Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return ReadAs<std::int8_t>(value, reader);
    } else if (prefix == EncodingByte::I16) {
      return ReadAs<std::int16_t>(value, reader);
    } else if (prefix == EncodingByte::I32) {
      return ReadAs<std::int32_t>(value, reader);
    } else if (prefix == EncodingByte::I64) {
      return ReadAs<std::int64_t>(value, reader);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// float encoding format:
// +-----+---//----+
// | FLT | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<float> : EncodingIO<float> {
  static constexpr EncodingByte Prefix(float /*value*/) {
    return EncodingByte::F32;
  }

  static constexpr std::size_t Size(float value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::F32;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             float value, Writer* writer) {
    return WriteAs<float>(value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            float* value, Reader* reader) {
    return ReadAs<float>(value, reader);
  }
};

//
// double encoding format:
// +-----+---//----+
// | DBL | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<double> : EncodingIO<double> {
  static constexpr EncodingByte Prefix(double /*value*/) {
    return EncodingByte::F64;
  }

  static constexpr std::size_t Size(double value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::F64;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             double value, Writer* writer) {
    return WriteAs<double>(value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            double* value, Reader* reader) {
    return ReadAs<double>(value, reader);
  }
};

//
// std::size_t encoding format depends on the size of std::size_t for the
// platform. Simply forward to either std::uint32_t or std::uint64_t.
//

template <typename T>
struct Encoding<T,
                std::enable_if_t<std::is_same<T, std::size_t>::value &&
                                 IsUnique<std::is_same, std::size_t,
                                          std::uint32_t, std::uint64_t>::value>>
    : EncodingIO<std::size_t> {
  // Check that std::size_t is either 32 or 64-bit.
  static_assert(sizeof(std::size_t) == sizeof(std::uint32_t) ||
                    sizeof(std::size_t) == sizeof(std::uint64_t),
                "std::size_t must be either 32 or 64-bit!");

  using BaseType =
      std::conditional_t<sizeof(std::size_t) == sizeof(std::uint32_t),
                         std::uint32_t, std::uint64_t>;

  static constexpr EncodingByte Prefix(std::size_t value) {
    return Encoding<BaseType>::Prefix(value);
  }

  static constexpr std::size_t Size(std::size_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<BaseType>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             std::size_t value,
                                             Writer* writer) {
    return Encoding<BaseType>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            std::size_t* value,
                                            Reader* reader) {
    BaseType base_value = 0;
    auto status = Encoding<BaseType>::ReadPayload(prefix, &base_value, reader);
    if (!status)
      return status;

    *value = base_value;
    return {};
  }
};

// Work around GCC bug that somtimes fails to match Endoding<int> to
// Encoding<std::int32_t>.
template <typename T>
struct Encoding<T, std::enable_if_t<std::is_same<T, int>::value &&
                                    sizeof(int) == sizeof(std::int32_t)>>
    : Encoding<std::int32_t> {};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_


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
// To handle the externally-defined structure use the macro
// NOP_EXTERNAL_STRUCTURE in the C++ code that handles serializing the data.
// Parenthesis are used to group the pair of members to treat as a logical
// buffer.
//
// NOP_EXTERNAL_STRUCTURE(SomeCType, (data, count));
//
// Example of defining a C++ type with a logical buffer pair:

// template <typename T>
// struct SomeTemplateType {
//  std::array<T, 20> elements;
//  std::size_t count;
//  NOP_STRUCTURE(SomeTemplateType, (elements, count));
// };
//
// Logical buffers are fungible with other array-like types:
//
// struct A {
//  int value;
//  std::vector<int> data;
//  NOP_STRUCTURE(A, value, data);
// };
//
// struct B {
//  int value;
//  int data[256];
//  size_t count;
//  NOP_STRUCTURE(B, value, (data, count));
// };
//
// static_assert(nop::IsFungible<A, B>::value, "!!");
//

namespace nop {

// Encoding type that handles non-integral element types. Logical buffers of
// non-integral element types are encoded the same as non-integral arrays using
// the ARRAY encoding.
template <typename BufferType, typename SizeType, bool IsUnbounded>
struct Encoding<
    LogicalBuffer<BufferType, SizeType, IsUnbounded>,
    EnableIfNotIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType, IsUnbounded>> {
  using Type = LogicalBuffer<BufferType, SizeType, IsUnbounded>;
  using ValueType = std::remove_const_t<typename Type::ValueType>;
  enum : std::size_t { Length = Type::Length };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    std::size_t element_size_sum = 0;
    for (const ValueType& element : value)
      element_size_sum += Encoding<ValueType>::Size(element);

    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) + element_size_sum;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const SizeType size = static_cast<SizeType>(value.size());
    if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status = Encoding<SizeType>::Write(size, writer);
    if (!status)
      return status;

    for (SizeType i = 0; i < size; i++) {
      status = Encoding<ValueType>::Write(value[i], writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    for (SizeType i = 0; i < size; i++) {
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
template <typename BufferType, typename SizeType, bool IsUnbounded>
struct Encoding<LogicalBuffer<BufferType, SizeType, IsUnbounded>,
                EnableIfIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType, IsUnbounded>> {
  using Type = LogicalBuffer<BufferType, SizeType, IsUnbounded>;
  using ValueType = std::remove_const_t<typename Type::ValueType>;
  enum : std::size_t { Length = Type::Length };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = value.size() * sizeof(ValueType);
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(size) +
           size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const SizeType size = value.size();
    if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status = Encoding<SizeType>::Write(size * sizeof(ValueType), writer);
    if (!status)
      return status;

    return writer->Write(value.begin(), value.end());
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size_bytes = 0;
    auto status = Encoding<SizeType>::Read(&size_bytes, reader);
    if (!status) {
      return status;
    } else if ((!IsUnbounded && size_bytes > Length * sizeof(ValueType)) ||
               size_bytes % sizeof(ValueType) != 0) {
      return ErrorStatus::InvalidContainerLength;
    }

    const SizeType size = size_bytes / sizeof(ValueType);
    value->size() = size;
    return reader->Read(value->begin(), value->end());
  }
};

}  // namespace nop

#endif  //  LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_




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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_
#define LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_

#include <functional>
#include <tuple>



/*
 * Copyright 2019 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_

#include <type_traits>



namespace nop {

namespace detail {

template <template <typename...> class Trait, typename Expression,
          typename... Args>
struct IsDetectedType : std::false_type {};

template <template <typename...> class Trait, typename... Args>
struct IsDetectedType<Trait, Void<Trait<Args...>>, Args...> : std::true_type {};

}  // namespace detail

// Trait that determines whether the given Trait template is successfully
// evaluated with the given Args. This makes it possible to express SFINAE
// detection expressions as templated aliases instead of full structure
// definitions with partial specializations.
//
// For example, a callable detector may be implemented as follows:
//
// template <typename Callable, typename... Args>
// using CallableTest =
//     decltype(std::declval<Callable>()(std::declval<Args>()...));
//
// template <typename Callable, typename... Args>
// using IsCallable = IsDetected<CallableTest, Callable, Args...>;
//
template <template <typename...> class Trait, typename... Args>
using IsDetected = detail::IsDetectedType<Trait, void, Args...>;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_



namespace nop {

// Captures the type and value of a pointer to member.
template <typename T, T, typename U = void*, U = nullptr,
          typename Enable = void>
struct MemberPointer;

template <typename T, typename Class, T Class::*Pointer>
struct MemberPointer<T Class::*, Pointer> {
  // Type of the memebr pointed to by this pointer.
  using Type = T;

  // Resolves a pointer to member with the given instance, yielding a pointer or
  // reference to the member in that instnace.
  static constexpr Type* Resolve(Class* instance) {
    return &(instance->*Pointer);
  }
  static constexpr const Type& Resolve(const Class& instance) {
    return (instance.*Pointer);
  }

  static constexpr std::size_t Size(const Class& instance) {
    return Encoding<Type>::Size(Resolve(instance));
  }

  template <typename Writer, typename MemberList>
  static constexpr Status<void> Write(const Class& instance, Writer* writer,
                                      MemberList /*member_list*/) {
    return Encoding<Type>::Write(Resolve(instance), writer);
  }

  template <typename Writer, typename MemberList>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const Class& instance,
                                             Writer* writer,
                                             MemberList /*member_list*/) {
    return Encoding<Type>::WritePayload(prefix, Resolve(instance), writer);
  }

  template <typename Reader, typename MemberList>
  static constexpr Status<void> Read(Class* instance, Reader* reader,
                                     MemberList /*member_list*/) {
    return Encoding<Type>::Read(Resolve(instance), reader);
  }

  template <typename Reader, typename MemberList>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            Class* instance, Reader* reader,
                                            MemberList /*member_list*/) {
    return Encoding<Type>::ReadPayload(prefix, Resolve(instance), reader);
  }
};

// Test expression for the external unbounded logical buffer tag.
template <typename Class>
using ExternalUnboundedBufferTest =
    decltype(NOP__GetUnboundedBuffer(std::declval<Class*>()));

// Test expression for the internal unbounded logical buffer tag.
template <typename Class>
using InternalUnboundedBufferTest = typename Class::NOP__UNBOUNDED_BUFFER;

// Evaluates to true if Class is tagged as an unbounded logical buffer.
template <typename Class>
struct IsUnboundedBuffer : Or<IsDetected<ExternalUnboundedBufferTest, Class>,
                              IsDetected<InternalUnboundedBufferTest, Class>> {
};

// Member pointer type for logical buffers formed by an array and size member
// pair.
template <typename Class, typename First, typename Second,
          First Class::*FirstPointer, Second Class::*SecondPointer>
struct MemberPointer<First Class::*, FirstPointer, Second Class::*,
                     SecondPointer, EnableIfLogicalBufferPair<First, Second>> {
  using Type = LogicalBuffer<First, Second, IsUnboundedBuffer<Class>::value>;

  static constexpr const Type Resolve(const Class& instance) {
    return {const_cast<First&>(instance.*FirstPointer),
            const_cast<Second&>(instance.*SecondPointer)};
  }

  static constexpr Type Resolve(Class* instance) {
    return {instance->*FirstPointer, instance->*SecondPointer};
  }

  static constexpr std::size_t Size(const Class& instance) {
    const Type pair = Resolve(instance);
    return Encoding<Type>::Size(pair);
  }

  template <typename Writer, typename MemberList>
  static constexpr Status<void> Write(const Class& instance, Writer* writer,
                                      MemberList /*member_list*/) {
    const Type pair = Resolve(instance);
    return Encoding<Type>::Write(pair, writer);
  }

  template <typename Writer, typename MemberList>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const Class& instance,
                                             Writer* writer,
                                             MemberList /*member_list*/) {
    const Type pair = Resolve(instance);
    return Encoding<Type>::WritePayload(prefix, pair, writer);
  }

  template <typename Reader, typename MemberList>
  static constexpr Status<void> Read(Class* instance, Reader* reader,
                                     MemberList /*member_list*/) {
    Type pair = Resolve(instance);
    return Encoding<Type>::Read(&pair, reader);
  }

  template <typename Reader, typename MemberList>
  static constexpr Status<void> ReadPayload(EncodingByte prefix,
                                            Class* instance, Reader* reader,
                                            MemberList /*member_list*/) {
    Type pair = Resolve(instance);
    return Encoding<Type>::ReadPayload(prefix, &pair, reader);
  }
};

// Captures a list of MemberPointers.
template <typename... MemberPointers>
struct MemberList {
  using Members = std::tuple<MemberPointers...>;

  enum : std::size_t { Count = sizeof...(MemberPointers) };

  template <std::size_t Index>
  using At = typename std::tuple_element<Index, Members>::type;
};

// Utility to retrieve a traits type that defines a MemberList for type T using
// ADL. The macros NOP_STRUCTURE, NOP_EXTERNAL_STRUCTURE, and
// NOP_EXTERNAL_TEMPLATE define the appropriate traits type and a defintion of
// NOP__GetExternalMemberTraits that this utility finds using ADL.
template <typename T>
using ExternalMemberTraits =
    decltype(NOP__GetExternalMemberTraits(std::declval<T*>()));

// Work around access check bug in GCC. Keeping original code here to document
// the desired behavior. Bug filed with GCC:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82478
#if 0
// Determines whether type T has a nested type named NOP__MEMBERS of
// template type MemberList.
template <typename, typename = void>
struct HasInternalMemberList : std::false_type {};
template <typename T>
struct HasInternalMemberList<T, Void<typename T::NOP__MEMBERS>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<MemberList, typename T::NOP__MEMBERS>::value> {
  static_assert(std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};
#else
// Determines whether type T has a nested type named NOP__MEMBERS of
// template type MemberList.
template <typename T, typename = void>
struct HasInternalMemberList {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__MEMBERS*) {
    return IsTemplateBaseOf<MemberList, typename U::NOP__MEMBERS>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };

  // Always true if T does not have a NOP__MEMBERS member type. If T does have
  // the member type then only true if T is also default constructible.
  static_assert(!value || std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};
#endif

// Determines whether type T has a properly defined traits type that can be
// discovered by ExternalMemberTraits above.
template <typename, typename = void>
struct HasExternalMemberList : std::false_type {};
template <typename T>
struct HasExternalMemberList<T,
                             Void<typename ExternalMemberTraits<T>::MemberList>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<MemberList, typename ExternalMemberTraits<
                                                 T>::MemberList>::value> {
  static_assert(std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};

// Determines whether a type has either an internal or external MemberList as
// defined by the predicates above.
template <typename T>
struct HasMemberList
    : std::integral_constant<bool, HasInternalMemberList<T>::value ||
                                       HasExternalMemberList<T>::value> {};

// Enable utilities for member list predicates.
template <typename T, typename ReturnType = void>
using EnableIfHasInternalMemberList =
    typename std::enable_if<HasInternalMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfHasExternalMemberList =
    typename std::enable_if<HasExternalMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfHasMemberList =
    typename std::enable_if<HasMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfNotHasMemberList =
    typename std::enable_if<!HasMemberList<T>::value, ReturnType>::type;

// Traits type that retrieves the internal or external MemberList associated
// with type T.
template <typename T, typename = void>
struct MemberListTraits;
template <typename T>
struct MemberListTraits<T, EnableIfHasInternalMemberList<T>> {
  using MemberList = typename T::NOP__MEMBERS;
};
template <typename T>
struct MemberListTraits<T, EnableIfHasExternalMemberList<T>> {
  using MemberList = typename ExternalMemberTraits<T>::MemberList;
};

// Determines whether type T has a nested type named NOP__VALUE of
// template type MemberList.
template <typename T, typename = void>
struct IsValueWrapper {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__VALUE*) {
    return IsTemplateBaseOf<MemberList, typename U::NOP__VALUE>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };
};

// Enable utilities for value wrapper predicates.
template <typename T, typename ReturnType = void>
using EnableIfIsValueWrapper =
    typename std::enable_if<IsValueWrapper<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfIsNotValueWrapper =
    typename std::enable_if<!IsValueWrapper<T>::value, ReturnType>::type;

// Traits type that retrieves the internal MemberList and Pointer associated
// with type T.
template <typename T, typename = void>
struct ValueWrapperTraits;
template <typename T>
struct ValueWrapperTraits<T, EnableIfIsValueWrapper<T>> {
  using MemberList = typename T::NOP__VALUE;
  using Pointer = typename MemberList::template At<0>;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_


namespace nop {

//
// struct/class T encoding format:
//
// +-----+---------+-----//----+
// | STC | INT64:N | N MEMBERS |
// +-----+---------+-----//----+
//
// Members must be valid encodings of their member type.
//

template <typename T>
struct Encoding<T, EnableIfHasMemberList<T>> : EncodingIO<T> {
  static constexpr EncodingByte Prefix(const T& /*value*/) {
    return EncodingByte::Structure;
  }

  static constexpr std::size_t Size(const T& value) {
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(Count) +
           Size(value, Index<Count>{});
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Structure;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const T& value, Writer* writer) {
    auto status = Encoding<SizeType>::Write(Count, writer);
    if (!status)
      return status;
    else
      return WriteMembers(value, writer, Index<Count>{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/, T* value,
                                            Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Count)
      return ErrorStatus::InvalidMemberCount;
    else
      return ReadMembers(value, reader, Index<Count>{});
  }

 private:
  enum : std::size_t { Count = MemberListTraits<T>::MemberList::Count };

  using MemberList = typename MemberListTraits<T>::MemberList;

  template <std::size_t Index>
  using PointerAt = typename MemberList::template At<Index>;

  static constexpr std::size_t Size(const T& /*value*/, Index<0>) { return 0; }

  template <std::size_t index>
  static constexpr std::size_t Size(const T& value, Index<index>) {
    return Size(value, Index<index - 1>{}) + PointerAt<index - 1>::Size(value);
  }

  template <typename Writer>
  static constexpr Status<void> WriteMembers(const T& /*value*/,
                                             Writer* /*writer*/, Index<0>) {
    return {};
  }

  template <std::size_t index, typename Writer>
  static constexpr Status<void> WriteMembers(const T& value, Writer* writer,
                                             Index<index>) {
    auto status = WriteMembers(value, writer, Index<index - 1>{});
    if (!status)
      return status;
    else
      return PointerAt<index - 1>::Write(value, writer, MemberList{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadMembers(T* /*value*/, Reader* /*reader*/,
                                            Index<0>) {
    return {};
  }

  template <std::size_t index, typename Reader>
  static constexpr Status<void> ReadMembers(T* value, Reader* reader,
                                            Index<index>) {
    auto status = ReadMembers(value, reader, Index<index - 1>{});
    if (!status)
      return status;
    else
      return PointerAt<index - 1>::Read(value, reader, MemberList{});
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_TABLE_H_
#define LIBNOP_INCLUDE_NOP_BASE_TABLE_H_




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

#ifndef LIBNOP_INCLUDE_NOP_TABLE_H_
#define LIBNOP_INCLUDE_NOP_TABLE_H_

#include <type_traits>

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_MACROS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MACROS_H_

// Macros to apply other macros over all elements in a list.

// Recursive expansion macros.
#define _NOP_EXPAND0(...) __VA_ARGS__
#define _NOP_EXPAND1(...) _NOP_EXPAND0(_NOP_EXPAND0(_NOP_EXPAND0(__VA_ARGS__)))
#define _NOP_EXPAND2(...) _NOP_EXPAND1(_NOP_EXPAND1(_NOP_EXPAND1(__VA_ARGS__)))
#define _NOP_EXPAND3(...) _NOP_EXPAND2(_NOP_EXPAND2(_NOP_EXPAND2(__VA_ARGS__)))
#define _NOP_EXPAND4(...) _NOP_EXPAND3(_NOP_EXPAND3(_NOP_EXPAND3(__VA_ARGS__)))
#define _NOP_EXPAND5(...) _NOP_EXPAND4(_NOP_EXPAND4(_NOP_EXPAND4(__VA_ARGS__)))
#define _NOP_EXPAND(...)  _NOP_EXPAND5(_NOP_EXPAND5(_NOP_EXPAND5(__VA_ARGS__)))

// Required to workaround a bug in the VC++ preprocessor.
#define _NOP_INDIRECT_EXPAND(macro, args) macro args

// Defines a step separation for macro expansion.
#define _NOP_SEPARATOR

// Clears any remaining contents wrapped in parentheses.
#define _NOP_CLEAR(...)

// Introduces a first dummy argument and _NOP_CLEAR as second argument.
#define _NOP_CLEAR_IF_LAST() _, _NOP_CLEAR

// Returns the first argument of a list.
#define _NOP_FIRST_ARG(first, ...) first

#define _NOP_REST_ARG(_, ...) __VA_ARGS__

// Returns the second argument of a list.
#define _NOP_SECOND_ARG(_, second, ...) second

#define _NOP_CAT(a, ...) a##__VA_ARGS__

#define _NOP_IS_PROBE(...) _NOP_SECOND_ARG(__VA_ARGS__, 0)
#define _NOP_PROBE() ~, 1

#define _NOP_IS_PAREN(...) _NOP_IS_PROBE(_NOP_IS_PAREN_PROBE __VA_ARGS__)
#define _NOP_IS_PAREN_PROBE(...) _NOP_PROBE()

#define _NOP_NOT(x) _NOP_IS_PROBE(_NOP_CAT(_NOP_NOT_, x))
#define _NOP_NOT_0 _NOP_PROBE()

#define _NOP_BOOL(x) _NOP_NOT(_NOP_NOT(x))

#define _NOP_IF_ELSE(condition) __NOP_IF_ELSE(_NOP_BOOL(condition))
#define __NOP_IF_ELSE(condition) _NOP_CAT(_NOP_IF_, condition)

#define _NOP_IF_1(...) __VA_ARGS__ _NOP_IF_1_ELSE
#define _NOP_IF_0(...) _NOP_IF_0_ELSE

#define _NOP_IF_1_ELSE(...)
#define _NOP_IF_0_ELSE(...) __VA_ARGS__

#define _NOP_HAS_ARGS(...) \
  _NOP_BOOL(_NOP_FIRST_ARG(_NOP_END_OF_ARGUMENTS_ __VA_ARGS__)())
#define _NOP_END_OF_ARGUMENTS_(...) _NOP_IS_PAREN(__VA_ARGS__)

#define _NOP_EMPTY()
#define _NOP_DEFER1(m) m _NOP_EMPTY()
#define _NOP_DEFER2(m) m _NOP_EMPTY _NOP_EMPTY()()
#define _NOP_DEFER3(m) m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()
#define _NOP_DEFER4(m) m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()()
#define _NOP_DEFER5(m) \
  m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()()()

#define _NOP_REMOVE_PARENS(...)            \
  _NOP_IF_ELSE(_NOP_IS_PAREN(__VA_ARGS__)) \
    (_NOP_STRIP_PARENS __VA_ARGS__)        \
    (__VA_ARGS__)

#define _NOP_STRIP_PARENS(...) __VA_ARGS__

#define NOP_MAP(...) _NOP_EXPAND(_NOP_MAP_FIRST(__VA_ARGS__))

#define _NOP_MAP_FIRST(m, ...)                         \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (           \
    m(_NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_MAP_NEXT(m, _NOP_REST_ARG(__VA_ARGS__))       \
  )(/* done */)

#define _NOP_MAP_NEXT(m, ...)                                    \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                     \
    , m(_NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__)))         \
    _NOP_DEFER3(__NOP_MAP_NEXT)()(m, _NOP_REST_ARG(__VA_ARGS__)) \
  )(/* done */)

#define __NOP_MAP_NEXT() _NOP_MAP_NEXT

#define NOP_MAP_ARGS(...) _NOP_EXPAND(_NOP_MAP_FIRST_ARGS(__VA_ARGS__))
#define _NOP_MAP_ARGS() NOP_MAP_ARGS

#define _NOP_MAP_FIRST_ARGS(m, args, ...)                                        \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                                     \
    m(_NOP_REMOVE_PARENS(args), _NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_MAP_NEXT_ARGS(m, args, _NOP_REST_ARG(__VA_ARGS__))                      \
  )(/* done */)

#define _NOP_MAP_NEXT_ARGS(m, args, ...)                                           \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                                       \
    , m(_NOP_REMOVE_PARENS(args), _NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_DEFER3(__NOP_MAP_NEXT_ARGS)()(m, args, _NOP_REST_ARG(__VA_ARGS__))        \
  )(/* done */)

#define __NOP_MAP_NEXT_ARGS() _NOP_MAP_NEXT_ARGS


#endif  // LIBNOP_INCLUDE_NOP_BASE_MACROS_H_

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

#ifndef LIBNOP_INCLUDE_NOP_STRUCTURE_H_
#define LIBNOP_INCLUDE_NOP_STRUCTURE_H_

#include <type_traits>




namespace nop {

//
// User-defined structures are structs or classes that have been annotated so
// that the serialization engine understands how to read and write them.
// Annotation is performed by invoking one of the following three macros:
//  * NOP_STRUCTURE in the body of the struct, class, or template.
//  * NOP_EXTERNAL_STRUCTURE at namespace scope matching the type.
//
// Example:
//
//  struct User {
//    std::string name;
//    std::uint8_t age;
//
//    enum class Gender {
//      Other,
//      Female,
//      Male,
//    };
//    nop::Optional<Gender> gender;
//
//    NOP_STRUCTURE(User, name, age, gender);
//  };
//

// Defines the set of members belonging to a type that should be
// serialized/deserialized. This macro must be invoked once within the
// struct/class definition, preferrably in the private section for classes with
// private data.
#define NOP_STRUCTURE(type, ... /*members*/)  \
  template <typename, typename>               \
  friend struct ::nop::Encoding;              \
  template <typename, typename>               \
  friend struct ::nop::HasInternalMemberList; \
  template <typename, typename>               \
  friend struct ::nop::MemberListTraits;      \
  using NOP__MEMBERS = ::nop::MemberList<_NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Defines the set of members belonging to a type that should be
// serialized/deserialized without changing the type itself. This is useful for
// making external library types with public data serializable.
#define NOP_EXTERNAL_STRUCTURE(type, ... /*members*/)                         \
  template <typename, typename>                                               \
  struct NOP__MEMBER_TRAITS;                                                  \
  template <typename T>                                                       \
  struct NOP__MEMBER_TRAITS<T, _NOP_ENABLE_IF_TYPE_MATCH(T, type)> {          \
    using MemberList = ::nop::MemberList<_NOP_MEMBER_LIST(T, __VA_ARGS__)>;   \
  };                                                                          \
  template <template <typename...> class TT, typename... Ts>                  \
  struct NOP__MEMBER_TRAITS<TT<Ts...>,                                        \
                            _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...)> { \
    using MemberList =                                                        \
        ::nop::MemberList<_NOP_MEMBER_LIST(TT<Ts...>, __VA_ARGS__)>;          \
  };                                                                          \
  template <typename T>                                                       \
  inline _NOP_ENABLE_IF_TYPE_MATCH(T, type, NOP__MEMBER_TRAITS<T, void>)      \
      NOP__GetExternalMemberTraits [[gnu::used]] (T*) {                       \
    return {};                                                                \
  }                                                                           \
  template <template <typename...> class TT, typename... Ts>                  \
  inline _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...,                       \
                                       NOP__MEMBER_TRAITS<TT<Ts...>, void>)   \
      NOP__GetExternalMemberTraits [[gnu::used]] (TT<Ts...>*) {               \
    return {};                                                                \
  }

// Deprecated. NOP_EXTERNAL_STRUCTURE can handle both type and template
// arguments now. Aliases NOP_EXTERNAL_STRUCTURE for legacy code.
#define NOP_EXTERNAL_TEMPLATE NOP_EXTERNAL_STRUCTURE

// Tags the given type as an unbounded buffer. This macro must be invoked once
// within the struct/class definition, preferrably in the private section next
// to NOP_STRUCTURE.
#define NOP_UNBOUNDED_BUFFER(type) using NOP__UNBOUNDED_BUFFER = type

// Tags the given type as an unbounded buffer without changing the type itself.
#define NOP_EXTERNAL_UNBOUNDED_BUFFER(type)                                    \
  template <typename, typename>                                                \
  struct NOP__UNBOUNDED_BUFFER;                                                \
  template <typename T>                                                        \
  struct NOP__UNBOUNDED_BUFFER<T, _NOP_ENABLE_IF_TYPE_MATCH(T, type)> {};      \
  template <template <typename...> class TT, typename... Ts>                   \
  struct NOP__UNBOUNDED_BUFFER<TT<Ts...>, _NOP_ENABLE_IF_TEMPLATE_MATCH(       \
                                              TT, type, Ts...)> {};            \
  template <typename T>                                                        \
  inline _NOP_ENABLE_IF_TYPE_MATCH(T, type, NOP__UNBOUNDED_BUFFER<T, void>)    \
      NOP__GetUnboundedBuffer [[gnu::used]] (T*) {                             \
    return {};                                                                 \
  }                                                                            \
  template <template <typename...> class TT, typename... Ts>                   \
  inline _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...,                        \
                                       NOP__UNBOUNDED_BUFFER<TT<Ts...>, void>) \
      NOP__GetUnboundedBuffer [[gnu::used]] (TT<Ts...>*) {                     \
    return {};                                                                 \
  }

//
// Utility macros used by the macros above.
//

// Enable if |type| is a type or elaborated template type matching type T.
#define _NOP_ENABLE_IF_TYPE_MATCH(T, type, ... /*Return*/) \
  std::enable_if_t<decltype(::nop::MatchType<T, type>())::value, ##__VA_ARGS__>

// Enable if |type| is an un-elaborated template type matching type TT.
#define _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts, ... /*Return*/)       \
  std::enable_if_t<decltype(::nop::MatchTemplate<TT, type, Ts>())::value, \
                   ##__VA_ARGS__>

// Generates a pair of template arguments (member pointer type and value) to be
// passed to MemberPointer<MemberPointerType, MemberPointerValue, ...> from the
// given type name and member name.
#define _NOP_MEMBER_POINTER(type, member) decltype(&type::member), &type::member

// Generates a MemberPointer type definition, given a type name and a variable
// number of member names. The first member name is handled here, while the
// remaining member names are passed to _NOP_MEMBER_POINTER_NEXT for recursive
// expansion.
#define _NOP_MEMBER_POINTER_FIRST(type, ...)                                  \
  ::nop::MemberPointer<_NOP_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
                           _NOP_MEMBER_POINTER_NEXT(                          \
                               type, _NOP_REST_ARG(__VA_ARGS__))>

// Recursively handles the remaining member names in the template argument list
// for MemberPointer.
#define _NOP_MEMBER_POINTER_NEXT(type, ...)                 \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__))                  \
  (, _NOP_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
         _NOP_DEFER3(__NOP_MEMBER_POINTER_NEXT)()(          \
             type, _NOP_REST_ARG(__VA_ARGS__)))(/*done*/)

// Indirection to enable recursive macro expansion of _NOP_MEMBER_POINTER_NEXT.
#define __NOP_MEMBER_POINTER_NEXT() _NOP_MEMBER_POINTER_NEXT

// Defines a list of MemberPointer types given a type and list of member names.
#define _NOP_MEMBER_LIST(type, ...) \
  NOP_MAP_ARGS(_NOP_MEMBER_POINTER_FIRST, (type), __VA_ARGS__)

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_STRUCTURE_H_

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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_

#include <initializer_list>
#include <type_traits>
#include <utility>

/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_

#include <type_traits>



namespace nop {

// Trait that determines whether the given types T and U have a defined
// equal comparision operator with type T on the left and type U on the right.
template <typename T, typename U, typename = void>
struct IsComparableEqual : std::false_type {};
template <typename T, typename U>
struct IsComparableEqual<
    T, U, Void<decltype(std::declval<const T&>() == std::declval<const U&>())>>
    : std::true_type {};

// Enable if T is comparable to U for equality.
template <typename T, typename U, typename Return = void>
using EnableIfComparableEqual =
    typename std::enable_if<IsComparableEqual<T, U>::value, Return>::type;

// Trait that determines whether the given types T and U have a defined
// less than comparision operator with type T on the left and type U on the
// right.
template <typename T, typename U, typename = void>
struct IsComparableLess : std::false_type {};
template <typename T, typename U>
struct IsComparableLess<
    T, U, Void<decltype(std::declval<const T&>() < std::declval<const U&>())>>
    : std::true_type {};

// Enable if T is comparable to U for less than inequality.
template <typename T, typename U, typename Return = void>
using EnableIfComparableLess =
    typename std::enable_if<IsComparableLess<T, U>::value, Return>::type;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_



namespace nop {

// Type tag to disambiguate in-place constructors.
struct InPlace {};

// Optional single-value container type. This type may either be empty or
// contain a valid instance of type T.
//
// Optional<T> has several useful properties:
//   1. An empty Optional<T> does not default construct its internal T value,
//      instead leaving the memory uninitialized via union semantics. This
//      avoids unnecessary initialization costs, and dynamic allocation in the
//      case that type T performs internal dynamic allocation when constructed.
//   2. Optional<T> adds only one byte to the static size of type T. Alignment
//      requirements may still result in additional padding.
//   3. When serialized, an empty Optional<T> may be more compact than the most
//      minimal encoding of type T. For example, when T is std::array<char, 128>
//      the minimal encoding of T is 130 bytes, making Optional<T> a good option
//      to save space during serialization when the value is not always
//      necessary. Conversely, the minimal encoding of an empty std::string is
//      two bytes, making Optional<T> less helpful for strings.
//   4. When type T is trivially destructible Optional<T> may be used in
//      constexpr expressions.
//
template <typename T>
class Optional {
  // Ensure that T is not InPlace.
  static_assert(!std::is_same<std::decay_t<T>, InPlace>::value,
                "T cannot be InPlace!");

  // Befriend other Optional types.
  template <typename U>
  friend class Optional;

 public:
  // Default constructs an empty Optional type.
  constexpr Optional() noexcept : state_{} {}

  // Constructs a non-empty Optional from lvalue and rvalue type T.
  constexpr Optional(const T& value) : state_{value} {}
  constexpr Optional(T&& value) : state_{std::move(value)} {}

  // Copy and move constructors.
  constexpr Optional(const Optional& other) : state_{other.state_} {}
  constexpr Optional(Optional&& other) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : state_{std::move(other.state_)} {}

  // Constructs a non-empty Optional from a type U such that T{U()} is valid.
  template <typename U, typename Enabled = std::enable_if_t<
                            std::is_constructible<T, U>::value>>
  constexpr Optional(U&& value) : state_{InPlace{}, std::forward<U>(value)} {}

  // In-place constructor with arbitrary argument forwarding.
  template <typename... Args>
  explicit constexpr Optional(InPlace, Args&&... args)
      : state_{InPlace{}, std::forward<Args>(args)...} {}

  // In-place constructor with initializer list and arbitrary argument
  // forwarding.
  template <typename U, typename... Args,
            typename Enabled = std::enable_if_t<
                std::is_constructible<T, std::initializer_list<U>>::value>>
  constexpr Optional(InPlace, std::initializer_list<U> il, Args&&... args)
      : state_{InPlace{}, il, std::forward<Args>(args)...} {}

  // Constructs a non-empty Optional from a compatible initializer list.
  template <typename U,
            typename Enabled = std::enable_if_t<
                std::is_constructible<T, std::initializer_list<U>>::value>>
  constexpr Optional(std::initializer_list<U> il) : state_{InPlace{}, il} {}

  // Default destructs the Optional. State and Storage nested types below handle
  // destruction for trivial and non-trivial types.
  ~Optional() = default;

  // Copy assignment operator.
  constexpr Optional& operator=(const Optional& other) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(other.state_.storage.value);
      } else {
        Destruct();
      }
    }
    return *this;
  }

  // Move assignment operator.
  constexpr Optional& operator=(Optional&& other) noexcept(
      std::is_nothrow_move_assignable<T>::value&&
          std::is_nothrow_move_constructible<T>::value) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(other.take());
        other.Destruct();
      } else {
        Destruct();
      }
    }
    return *this;
  }

  // Copy assignment from a different Optional type.
  template <typename U>
  constexpr std::enable_if_t<std::is_constructible<T, const U&>::value, Optional&>
  operator=(const Optional<U>& other) {
    if (!other.empty()) {
      Assign(other.get());
    } else {
      Destruct();
    }
    return *this;
  }

  // Move assignment from a different Optional type.
  template <typename U>
  constexpr std::enable_if_t<std::is_constructible<T, U&&>::value, Optional&> operator=(
      Optional<U>&& other) {
    if (!other.empty()) {
      Assign(other.take());
      other.Destruct();
    } else {
      Destruct();
    }
    return *this;
  }

  // Copy/move assignment from type U.
  template <typename U>
  std::enable_if_t<std::is_constructible<T, U>::value, Optional&> operator=(
      U&& value) {
    Assign(std::forward<U>(value));
    return *this;
  }

  // Returns true if the Optional is empty.
  constexpr bool empty() const noexcept { return state_.empty; }

  // Returns true if the Optional is non-empty.
  explicit constexpr operator bool() const noexcept { return !empty(); }

  // Returns the underlying value. These accessors may only be called when
  // non-empty.
  constexpr const T& get() const { return state_.storage.value; }
  constexpr T& get() { return state_.storage.value; }
  constexpr T&& take() { return std::move(state_.storage.value); }

  // Clears the optional to the empty state, destroying the underlying value if
  // necessary.
  void clear() { Destruct(); }

 private:
  // Handles assignment/construction for assignment operators.
  template <typename U>
  constexpr void Assign(U&& value) {
    if (empty()) {
      ::new (&state_.storage.value) T(std::forward<U>(value));
      state_.empty = false;
    } else {
      state_.storage.value = std::forward<U>(value);
    }
  }

  // Destroys the stored value if non-empty.
  constexpr void Destruct() noexcept {
    if (!empty()) {
      state_.storage.value.T::~T();
      state_.empty = true;
    }
  }

  // Type tag used by the nested State type to explicitly select the trivial
  // constructor in the nested Storage type.
  struct TrivialInit {};

  // Base union type used to implement lazy-initialized storage. This type is
  // specialized to handle trivially and non-trivially destructible types.
  template <typename U, typename Enabled = void>
  union Storage;

  // Specialization that handles non-trivially destructible types.
  template <typename U>
  union Storage<U,
                std::enable_if_t<!std::is_trivially_destructible<U>::value>> {
    // Dummy type that is active when the value type is inactive. Since this
    // type is trivially-destructible no special care is necessary when
    // activating the value type.
    unsigned char dummy;

    // Value type that is active when the dummy type is inactive. This type must
    // be properly destroyed before deactivation.
    U value;

    // Trivial constructor that makes the dummy type active.
    constexpr Storage(TrivialInit) noexcept : dummy{} {}

    // Constructs the value type, making it active.
    template <typename... Args>
    constexpr Storage(Args&&... args) noexcept
        : value(std::forward<Args>(args)...) {}

    // Non-trivial destructor. This doesn't do anything useful except enable to
    // compiler to catch attempts to use this type in a non-trivial context.
    ~Storage() {}
  };

  // Specialization that handles trivially-destructible types.
  template <typename U>
  union Storage<U, std::enable_if_t<std::is_trivially_destructible<U>::value>> {
    // Dummy type that is active when the value type is inactive. Since this
    // type is trivially-destructible no special care is necessary when
    // activating the value type.
    unsigned char dummy;

    // Value type that is active when the dummy type is inactive. Since this
    // type is also trivially-destructible no special care is required at
    // deactivation.
    U value;

    // Trivial constructor that makes the dummy type active.
    constexpr Storage(TrivialInit) noexcept : dummy{} {}

    // Constructs the value type, making it active.
    template <typename... Args>
    constexpr Storage(Args&&... args) noexcept
        : value(std::forward<Args>(args)...) {}

    // Trivial destructor.
    ~Storage() = default;
  };

  // Base type to store a lazy-initialized value and track whether it is
  // initialized. This type is specialized to handle trivailly and non-trivially
  // destructible value types.
  template <typename U, typename Enabled = void>
  struct State;

  // Specialization that handles non-trivially destructible types.
  template <typename U>
  struct State<U, std::enable_if_t<!std::is_trivially_destructible<U>::value>> {
    // Default constructor that sets the state to empty and trivially
    // initializes the storage for the value type.
    constexpr State() noexcept : empty{true}, storage{TrivialInit{}} {}

    // Copy constructor. Copy constructs the value type if the other state is
    // non-empty.
    constexpr State(const State& other)
        : empty{other.empty}, storage{TrivialInit{}} {
      if (!other.empty)
        ::new (&storage.value) U(other.storage.value);
    }

    // Move constructor. Move constructs the value type if the other state is
    // non-empty.
    constexpr State(State&& other)
        : empty{other.empty}, storage{TrivialInit{}} {
      if (!other.empty)
        ::new (&storage.value) U(std::move(other.storage.value));
    }

    // Value constructors. Sets the state to non-empty and copies or moves the
    // value to storage.
    explicit constexpr State(const U& value) : empty{false}, storage{value} {}
    explicit constexpr State(U&& value)
        : empty{false}, storage{std::move(value)} {}

    // In-place constructor for more complex initialization.
    template <typename... Args>
    explicit State(InPlace, Args&&... args)
        : empty{false}, storage{std::forward<Args>(args)...} {}

    // In-place initializer list constructor.
    template <typename V, typename... Args,
              typename = std::enable_if_t<
                  std::is_constructible<U, std::initializer_list<V>>::value>>
    explicit State(InPlace, std::initializer_list<V> il, Args&&... args)
        : empty{false}, storage{il, std::forward<Args>(args)...} {}

    // Non-trivial destructor. Destroys the value in storage if non-empty.
    ~State() {
      if (!empty)
        storage.value.U::~U();
    }

    // Tracks whether the storage value is empty (un-initialized) or non-empty
    // (initialized).
    bool empty;

    // Storage for the lazy-initialized value.
    Storage<U> storage;
  };

  // Specialization that handles trivially-destructible types. This
  // specialization conforms with the requirements for constexpr expressions.
  template <typename U>
  struct State<U, std::enable_if_t<std::is_trivially_destructible<U>::value>> {
    // Default constructor that sets the state to empty and trivially
    // initializes the storage for the value type.
    constexpr State() noexcept : empty{true}, storage{TrivialInit{}} {}

    // Copy constructor. Copy constructs the value type if the other state is
    // non-empty.
    constexpr State(const State& other)
        : empty{other.empty}, storage{other.storage.value} {}

    // Move constructor. Move constructs the value type if the other state is
    // non-empty.
    constexpr State(State&& other)
        : empty{other.empty}, storage{std::move(other.storage.value)} {}

    // Value constructors. Sets the state to non-empty and copies or moves the
    // value to storage.
    explicit constexpr State(const U& value) : empty{false}, storage{value} {}
    explicit constexpr State(U&& value)
        : empty{false}, storage{std::move(value)} {}

    // In-place constructor for more complex initialization.
    template <typename... Args>
    explicit constexpr State(InPlace, Args&&... args)
        : empty{false}, storage{std::forward<Args>(args)...} {}

    // In-place initializer list constructor.
    template <typename V, typename... Args,
              typename = std::enable_if_t<
                  std::is_constructible<U, std::initializer_list<V>>::value>>
    explicit constexpr State(InPlace, std::initializer_list<V> il,
                             Args&&... args)
        : empty{false}, storage{il, std::forward<Args>(args)...} {}

    // Trivial destructor.
    ~State() = default;

    // Tracks whether the storage value is empty (un-initialized) or non-empty
    // (initialized).
    bool empty;

    // Storage for the lazy-initialized value.
    Storage<U> storage;
  };

  // Tracks the value and empty/non-empty state.
  State<T> state_;
};

// Relational operators.

template <typename T, typename U,
          typename Enabled = EnableIfComparableEqual<T, U>>
constexpr bool operator==(const Optional<T>& a, const Optional<U>& b) {
  if (a.empty() != b.empty())
    return false;
  else if (a.empty())
    return true;
  else
    return a.get() == b.get();
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableEqual<T, U>>
constexpr bool operator!=(const Optional<T>& a, const Optional<U>& b) {
  return !(a == b);
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator<(const Optional<T>& a, const Optional<U>& b) {
  if (b.empty())
    return false;
  else if (a.empty())
    return true;
  else
    return a.get() < b.get();
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator>(const Optional<T>& a, const Optional<U>& b) {
  return b < a;
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator<=(const Optional<T>& a, const Optional<U>& b) {
  return !(b < a);
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator>=(const Optional<T>& a, const Optional<U>& b) {
  return !(a < b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator==(const Optional<T>& a, const U& b) {
  return !a.empty() ? a.get() == b : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator==(const T& a, const Optional<U>& b) {
  return !b.empty() ? a == b.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator!=(const Optional<T>& a, const U& b) {
  return !(a == b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator!=(const T& a, const Optional<U>& b) {
  return !(a == b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator<(const Optional<T>& a, const U& b) {
  return !a.empty() ? a.get() < b : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator<(const T& a, const Optional<U>& b) {
  return !b.empty() ? a < b.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator>(const Optional<T>& a, const U& b) {
  return !a.empty() ? b < a.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator>(const T& a, const Optional<U>& b) {
  return !b.empty() ? b.get() < a : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator<=(const Optional<T>& a, const U& b) {
  return !a.empty() ? !(b < a.get()) : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator<=(const T& a, const Optional<U>& b) {
  return !b.empty() ? !(b.get() < a) : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator>=(const Optional<T>& a, const U& b) {
  return !a.empty() ? !(a.get() < b) : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator>=(const T& a, const Optional<U>& b) {
  return !b.empty() ? !(a < b.get()) : true;
}

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_

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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_

#include <array>
#include <cstdint>

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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_

#define NOP_GCC_VERSION \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// Compatability with non-clang compilers.
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

// Test for fallthrough support.
#if __has_cpp_attribute(clang::fallthrough)
#define NOP_FALLTHROUGH [[clang::fallthrough]]
#elif NOP_GCC_VERSION >= 70000
#define NOP_FALLTHROUGH [[fallthrough]]
#else
#define NOP_FALLTHROUGH
#endif

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_


// A direct port of the SipHash C reference implementation.
//
// https://131002.net/siphash/siphash24.c
//
// This version supports compile-time constexpr hash computation when provided
// with a byte container that supports constexpr size() and operator[] methods.
//

namespace nop {

// A simple byte container/wrapper with constexpr size() and operator[] methods.
template <typename T>
class BlockReader {
 public:
  static_assert(sizeof(T) == 1, "sizeof(T) != 1");
  using ValueType = T;

  template <std::size_t Size>
  constexpr BlockReader(const ValueType (&value)[Size])
      : data_{value}, size_{Size} {}
  constexpr BlockReader(const ValueType* data, std::size_t size)
      : data_{data}, size_{size} {}

  BlockReader(const BlockReader&) = default;
  BlockReader& operator=(const BlockReader&) = default;

  constexpr std::size_t size() const { return size_; }
  constexpr ValueType operator[](const std::size_t index) const {
    return data_[index];
  }

 private:
  const ValueType* data_;
  std::size_t size_;
};

// Captures a hash value as a compile-time type.
template <std::uint64_t Hash_>
struct HashValue {
  enum : std::uint64_t { Value = Hash_ };
};

struct SipHash {
  template <typename T, std::size_t Size>
  static constexpr std::uint64_t Compute(const T (&buffer)[Size],
                                         std::uint64_t k0, std::uint64_t k1) {
    return Compute(BlockReader<T>(buffer), k0, k1);
  }

  template <typename BufferType>
  static constexpr std::uint64_t Compute(const BufferType buffer,
                                         std::uint64_t k0, std::uint64_t k1) {
    const std::size_t kBlockSize = sizeof(std::uint64_t);
    const std::size_t kLength = buffer.size();
    const std::size_t kLeftOver = kLength % kBlockSize;
    const std::size_t kEndOffset = kLength - kLeftOver;

    std::uint64_t v[4] = {0x736f6d6570736575ULL, 0x646f72616e646f6dULL,
                          0x6c7967656e657261ULL, 0x7465646279746573ULL};

    std::uint64_t b = static_cast<std::uint64_t>(kLength) << 56;

    v[3] ^= k1;
    v[2] ^= k0;
    v[1] ^= k1;
    v[0] ^= k0;

    for (std::size_t offset = 0; offset < kEndOffset; offset += kBlockSize) {
      std::uint64_t m = ReadBlock(buffer, offset);
      v[3] ^= m;
      Round(v);
      Round(v);
      v[0] ^= m;
    }

    switch (kLeftOver) {
      case 7:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 6]) << 48;
        NOP_FALLTHROUGH;
      case 6:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 5]) << 40;
        NOP_FALLTHROUGH;
      case 5:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 4]) << 32;
        NOP_FALLTHROUGH;
      case 4:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 3]) << 24;
        NOP_FALLTHROUGH;
      case 3:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 2]) << 16;
        NOP_FALLTHROUGH;
      case 2:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 1]) << 8;
        NOP_FALLTHROUGH;
      case 1:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 0]) << 0;
        NOP_FALLTHROUGH;
      case 0:
        break;
    }

    v[3] ^= b;
    Round(v);
    Round(v);
    v[0] ^= b;

    v[2] ^= 0xff;
    Round(v);
    Round(v);
    Round(v);
    Round(v);
    b = v[0] ^ v[1] ^ v[2] ^ v[3];

    return b;
  }

 private:
  template <typename BufferType>
  static constexpr std::uint64_t ReadBlock(const BufferType buffer,
                                           const std::size_t offset) {
    const std::uint64_t v0 = buffer[offset + 0];
    const std::uint64_t v1 = buffer[offset + 1];
    const std::uint64_t v2 = buffer[offset + 2];
    const std::uint64_t v3 = buffer[offset + 3];
    const std::uint64_t v4 = buffer[offset + 4];
    const std::uint64_t v5 = buffer[offset + 5];
    const std::uint64_t v6 = buffer[offset + 6];
    const std::uint64_t v7 = buffer[offset + 7];

    return ((v7 << 56) | (v6 << 48) | (v5 << 40) | (v4 << 32) | (v3 << 24) |
            (v2 << 16) | (v1 << 8) | (v0 << 0));
  }

  static constexpr std::uint64_t RotateLeft(const std::uint64_t x,
                                            const std::uint64_t b) {
    return (x << b) | (x >> (64 - b));
  }

  static constexpr void Round(std::uint64_t (&v)[4]) {
    v[0] += v[1];
    v[1] = RotateLeft(v[1], 13);
    v[1] ^= v[0];
    v[0] = RotateLeft(v[0], 32);
    v[2] += v[3];
    v[3] = RotateLeft(v[3], 16);
    v[3] ^= v[2];
    v[0] += v[3];
    v[3] = RotateLeft(v[3], 21);
    v[3] ^= v[0];
    v[2] += v[1];
    v[1] = RotateLeft(v[1], 17);
    v[1] ^= v[2];
    v[2] = RotateLeft(v[2], 32);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_


namespace nop {

//
// Tables are bi-directional binary compatible structures that support
// serializing and deserializing data from different versions of the same type.
// Use a table type when maintaining compatability between different versions of
// serialized data is important to the application. However, consider that every
// non-empty table entry cost at least two bytes more than the underlying type
// encoding.
//
// Users define tables using classes or structures with members of type
// Entry<T, Id>. Entry<T, Id> is similar to Optional<T> in that an entry may
// either be empty or contain a value of type T. Entries that are empty are not
// encoded during serialization to save space. Programs using tables should
// handle empty values in a sensible way, ensuring that missing entries in older
// data are handled gracefully.
//
// Example of a simple table definition:
//
// struct MyTable {
//   Entry<Address, 0> address;
//   Entry<PhoneNumber, 1> phone_number;
//   NOP_TABLE(MyTable, address, phone_number);
// };
//
// Example of handling empty values:
//
// MyTable my_table;
// auto status = serializer.Read(&my_table);
// if (!status)
//   return status;
//
// if (my_table.address)
//   HandleAddress(my_table.address.get());
// if (my_table.phone_number)
//   HandlePhoneNumber(my_table.phone_number.get());
//
// Table entries may be public, private, or protected, depending on how the
// table type will be used. If the entries are non-public, call NOP_TABLE*() in
// a private section to avoid exposing member pointers to arbitrary code.
//
// Use the following rules to maximize compatability between different versions
// of a table type:
//   1. Always use unique values for Id when adding an entry to a table. Never
//      reuse a previously used Id in the same table.
//   2. When deprecating an entry use the DeletedEntry type instead of deleting
//      the entry entirely to document the deprecation and prevent resuse of an
//      old entry id.
//   3. Never change the Id for an entry. Doing so will break compatibility with
//      older versions of serialized data.
//   4. Never change the namespace hash or namespace string passed as the first
//      argument to the macros NOP_TABLE_HASH / NOP_TABLE_NS. These values are
//      used for sanity checking during deserialization. Changing these
//      arguments will break compatibility with older versions of serialized
//      data.
//

// Type tags to define used and deprecated entries.
struct ActiveEntry {};
struct DeletedEntry {};

// Base type of table entries.
template <typename, std::uint64_t, typename = ActiveEntry>
struct Entry;

// Specialization of Entry for active/used entries. Each non-empty, active entry
// is encoded during serialization.
template <typename T, std::uint64_t Id_>
struct Entry<T, Id_, ActiveEntry> : Optional<T> {
  enum : std::uint64_t { Id = Id_ };
  using Optional<T>::Optional;
  using Optional<T>::operator=;
};

// Specialization of Entry for deleted/deprecated entries. These entries are
// always empty and are never encoded. When encountered during deserialization
// these entries are ignored.
template <typename T, std::uint64_t Id_>
struct Entry<T, Id_, DeletedEntry> {
  enum : std::uint64_t { Id = Id_ };

  bool empty() const { return true; }
  explicit operator bool() const { return !empty(); }
  void clear() {}
};

// Defines a table type, its namespace hash, and its members. This macro must be
// invoked once within a table struct or class to inform the serialization
// engine about the table members and hash value. The macro befriends several
// key classes and defines an internal type named NOP__ENTRIES that describes
// the table type's Entry<T, Id> members to the engine.
#define NOP_TABLE_HASH(hash, type, ... /*entries*/)             \
  template <typename, typename>                                 \
  friend struct ::nop::Encoding;                                \
  template <typename, typename>                                 \
  friend struct ::nop::HasEntryList;                            \
  template <typename>                                           \
  friend struct ::nop::EntryListTraits;                         \
  using NOP__ENTRIES = ::nop::EntryList<::nop::HashValue<hash>, \
                                        _NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Similar to NOP_TABLE_HASH except that the namespace hash is computed from a
// compile-time hash of the given string literal that defines the namespace of
// the table.
#define NOP_TABLE_NS(string_name, type, ... /*entries*/)                    \
  NOP_TABLE_HASH(::nop::SipHash::Compute(string_name, ::nop::kNopTableKey0, \
                                         ::nop::kNopTableKey1),             \
                 type, __VA_ARGS__)

// Similar to NOP_TABLE_HASH except that the namespace hash is set to zero,
// which has a compact encoding compared to average 64bit hash values. This
// saves space in the encoding when namespace checks are not desired.
#define NOP_TABLE(type, ... /*entries*/) NOP_TABLE_HASH(0, type, __VA_ARGS__)

// Determines whether two entries have the same id.
template <typename A, typename B>
struct SameEntryId : std::integral_constant<bool, A::Id == B::Id> {};

// Similar to MemberList used for serializable structures/classes. This type
// also records the hash of the table name for sanity checking during
// deserialization.
template <typename HashValue, typename... MemberPointers>
struct EntryList : MemberList<MemberPointers...> {
  static_assert(IsUnique<SameEntryId, typename MemberPointers::Type...>::value,
                "Entry ids must be unique.");
  enum : std::uint64_t { Hash = HashValue::Value };
};

// Work around access check bug in GCC. Keeping original code here to document
// the desired behavior. Bug filed with GCC:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82478
#if 0
// Determines whether the given type T has a nested type named NOP__ENTRIES that
// is a subtype of EntryList. This type evaluates to std::true_type for a
// properly defined table type, std::false_type otherwise.
template <typename, typename = void>
struct HasEntryList : std::false_type {};
template <typename T>
struct HasEntryList<T, Void<typename T::NOP__ENTRIES>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<EntryList, typename T::NOP__ENTRIES>::value> {
};
#else
// Determines whether the given type T has a nested type named NOP__ENTRIES that
// is a subtype of EntryList.
template <typename T, typename = void>
struct HasEntryList {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__ENTRIES*) {
    return IsTemplateBaseOf<EntryList, typename U::NOP__ENTRIES>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };
};
#endif

// Enable if HasEntryType<T> evaluates to true.
template <typename T>
using EnableIfHasEntryList =
    typename std::enable_if<HasEntryList<T>::value>::type;

// Traits type that retrieves the NOP__ENTRIES traits type for the given type T.
template <typename T>
struct EntryListTraits {
  using EntryList = typename T::NOP__ENTRIES;
};

// SipHash keys used to compute the table hash of the given table name string.
enum : std::uint64_t {
  kNopTableKey0 = 0xbaadf00ddeadbeef,
  kNopTableKey1 = 0x0123456789abcdef,
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TABLE_H_

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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>


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

#ifndef LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_


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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_TYPES_HANDLE_H_

#include <functional>
#include <type_traits>



namespace nop {

//
// Types for managing abstract resource objects. Examples of resource objects
// are QNX channels, UNIX file descriptors, and Android binders.
//
// The types here address two major concerns for resource objects:
//   1. The ownership of resource objects and their lifetime within the local
//      process.
//   2. Sharing of resource objects between processes, when supported.
//

// Reference type used by the Reader/Writer to reference handles in serialized form.
using HandleReference = std::int64_t;
enum : HandleReference { kEmptyHandleReference = -1 };

// Default handle policy. Primarily useful as an example of the required form of
// a handle policy.
template <typename T, T Empty = T{}>
struct DefaultHandlePolicy {
  // A handle policy must define a member type named Type as the value type of
  // the handle.
  using Type = T;

  // Returns an empty value used when the handle is empty.
  static constexpr T Default() { return Empty; }

  // Returns true if the handle is not empty.
  static bool IsValid(const T& value) { return value != Empty; }

  // Closes the handle and assigns the empty value.
  static void Close(T* value) { *value = Empty; }

  // Releases the value from the handle and assigns the empty value in its
  // place. The returned value is no longer managed by the handle.
  static T Release(T* value) {
    T temp{Empty};
    std::swap(*value, temp);
    return temp;
  }

  // Returns the handle type to use when encoding a handle.
  static constexpr std::uint64_t HandleType() { return 0; }
};

// Represents a handle to a resource object. The given policy type provides the
// underlying handle type and the operations necessary to managae the resource.
// This class does not explicitly manage the lifetime of the underlying
// resource: it is useful as an argument for functions that don't acquire
// ownership of the handle.
template <typename Policy>
class Handle {
 public:
  using Type = typename Policy::Type;

  template <typename T, typename Enabled = EnableIfConvertible<T, Type>>
  explicit Handle(T&& value) : value_{std::forward<T>(value)} {}

  Handle() : value_{Policy::Default()} {}
  Handle(const Handle&) = default;
  Handle& operator=(const Handle&) = default;

  explicit operator bool() const { return Policy::IsValid(value_); }
  const Type& get() const { return value_; }

 protected:
  Type value_;
};

// Represents a handle to a resource object with a managed lifetime and
// ownership.
template <typename Policy>
class UniqueHandle : public Handle<Policy> {
 public:
  using Type = typename Policy::Type;
  using Base = Handle<Policy>;

  template <typename T, typename Enabled = EnableIfConvertible<T, Type>>
  explicit UniqueHandle(T&& value) : Base{std::forward<T>(value)} {}

  UniqueHandle() = default;
  UniqueHandle(UniqueHandle&& other) : UniqueHandle() {
    *this = std::move(other);
  }
  UniqueHandle& operator=(UniqueHandle&& other) {
    if (this != &other) {
      close();
      std::swap(this->value_, other.value_);
    }
    return *this;
  }

  ~UniqueHandle() { close(); }

  void close() { Policy::Close(&this->value_); }
  Type release() { return Policy::Release(&this->value_); }

 private:
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_HANDLE_H_


namespace nop {

//
// Handle<Policy> encoding format:
//
// +-----+------+-----------+
// | HND | TYPE | INT64:REF |
// +-----+------+-----------+
//

template <typename Policy>
struct Encoding<Handle<Policy>> : EncodingIO<Handle<Policy>> {
  using Type = Handle<Policy>;
  using HandleType = decltype(Policy::HandleType());

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Handle;
  }

  static constexpr std::size_t Size(const Type& value) {
    // Overestimate the size as though the handle reference is I64 because the
    // handle reference value, and therefore size, is not known ahead of
    // serialization.
    return BaseEncodingSize(Prefix(value)) +
           Encoding<HandleType>::Size(Policy::HandleType()) +
           BaseEncodingSize(EncodingByte::I64);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Handle;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<HandleType>::Write(Policy::HandleType(), writer);
    if (!status)
      return status;

    auto push_status = writer->template PushHandle<Type>(value);
    if (!push_status)
      return push_status.error();

    return Encoding<HandleReference>::Write(push_status.get(), writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    HandleType handle_type;
    auto status = Encoding<HandleType>::Read(&handle_type, reader);
    if (!status)
      return status;
    else if (handle_type != Policy::HandleType())
      return ErrorStatus::UnexpectedHandleType;

    HandleReference handle_reference = kEmptyHandleReference;
    status = Encoding<HandleReference>::Read(&handle_reference, reader);
    if (!status)
      return status;

    auto get_status = reader->template GetHandle<Type>(handle_reference);
    if (!get_status)
      return get_status.error();

    *value = get_status.take();
    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_



namespace nop {

// BoundedReader is a reader type that wraps another reader pointer and tracks
// the number of bytes read. Reader operations are transparently passed to the
// underlying reader unless the requested operation would exceed the size limit
// set at construction. BufferReader can also skip padding remaining in the
// input up to the size limit in situations that require specific input payload
// size.
template <typename Reader>
class BoundedReader {
 public:
  constexpr BoundedReader() = default;
  constexpr BoundedReader(const BoundedReader&) = default;
  constexpr BoundedReader(Reader* reader, std::size_t size)
      : reader_{reader}, size_{size} {}

  constexpr BoundedReader& operator=(const BoundedReader&) = default;

  constexpr Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return reader_->Ensure(size);
  }

  constexpr Status<void> Read(std::uint8_t* byte) {
    if (index_ < size_) {
      auto status = reader_->Read(byte);
      if (!status)
        return status;

      index_ += 1;
      return {};
    } else {
      return ErrorStatus::ReadLimitReached;
    }
  }

  template <typename T, typename Enable = EnableIfArithmetic<T>>
  constexpr Status<void> Read(T* begin, T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    if (length_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    auto status = reader_->Read(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    auto status = reader_->Skip(padding_bytes);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  // Skips any bytes remaining in the limit set at construction.
  constexpr Status<void> ReadPadding() {
    const std::size_t padding_bytes = size_ - index_;
    auto status = reader_->Skip(padding_bytes);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  constexpr Status<HandleType> GetHandle(HandleReference handle_reference) {
    return reader_->GetHandle(handle_reference);
  }

  constexpr bool empty() const { return index_ == size_; }

  constexpr std::size_t size() const { return index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  Reader* reader_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_

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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>





namespace nop {

// BoundedWriter is a writer type that wraps another writer pointer and tracks
// the number of bytes written. Writer operations are transparently passed to
// the underlying writer unless the requested operation would exceed the size
// limit set at construction. BufferWriter can also pad the output up to the
// size limit in situations that require specific output payload size.
template <typename Writer>
class BoundedWriter {
 public:
  constexpr BoundedWriter() = default;
  constexpr BoundedWriter(const BoundedWriter&) = default;
  constexpr BoundedWriter(Writer* writer, std::size_t size)
      : writer_{writer}, size_{size} {}

  constexpr BoundedWriter& operator=(const BoundedWriter&) = default;

  constexpr Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return writer_->Prepare(size);
  }

  constexpr Status<void> Write(std::uint8_t byte) {
    if (index_ < size_) {
      auto status = writer_->Write(byte);
      if (!status)
        return status;

      index_ += 1;
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  template <typename T, typename Enabel = EnableIfArithmetic<T>>
  constexpr Status<void> Write(const T* begin, const T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    auto status = writer_->Write(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes,
                              std::uint8_t padding_value = 0x00) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  // Fills out any remaining bytes with the given padding value.
  constexpr Status<void> WritePadding(std::uint8_t padding_value = 0x00) {
    const std::size_t padding_bytes = size_ - index_;
    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  constexpr Status<HandleType> PushHandle(const HandleType& handle) {
    return writer_->PushHandle(handle);
  }

  constexpr std::size_t size() const { return index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  Writer* writer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_


namespace nop {

//
// Entry<T, Id, ActiveEntry> encoding format:
//
// +----------+------------+-------+---------+
// | INT64:ID | INT64:SIZE | VALUE | PADDING |
// +----------+------------+-------+---------+
//
// VALUE must be a valid encoding of type T. If the entry is empty it is not
// encoded. The encoding of type T is wrapped in a sized binary encoding to
// allow deserialization to skip unknown entry types without parsing the full
// encoded entry. SIZE is equal to the total number of bytes in VALUE and
// PADDING.
//
// Entry<T, Id, DeletedEntry> encoding format:
//
// +----------+------------+--------------+
// | INT64:ID | INT64:SIZE | OPAQUE BYTES |
// +----------+------------+--------------+
//
// A deleted entry is never written, but may be encountered by code using newer
// table definitions to read older data streams.
//
// Table encoding format:
//
// +-----+------------+---------+-----------+
// | TAB | INT64:HASH | INT64:N | N ENTRIES |
// +-----+------------+---------+-----------+
//
// Where HASH is derived from the table label and N is the number of non-empty,
// active entries in the table. Older code may encounter unknown entry ids when
// reading data from newer table definitions.
//

template <typename Table>
struct Encoding<Table, EnableIfHasEntryList<Table>> : EncodingIO<Table> {
  static constexpr EncodingByte Prefix(const Table& /*value*/) {
    return EncodingByte::Table;
  }

  static constexpr std::size_t Size(const Table& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(
               EntryListTraits<Table>::EntryList::Hash) +
           Encoding<SizeType>::Size(ActiveEntryCount(value, Index<Count>{})) +
           Size(value, Index<Count>{});
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Table;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Table& value,
                                             Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(
        EntryListTraits<Table>::EntryList::Hash, writer);
    if (!status)
      return status;

    status = Encoding<SizeType>::Write(ActiveEntryCount(value, Index<Count>{}),
                                       writer);
    if (!status)
      return status;

    return WriteEntries(value, writer, Index<Count>{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Table* value, Reader* reader) {
    // Clear entries so that we can detect whether there are duplicate entries
    // for the same id during deserialization.
    ClearEntries(value, Index<Count>{});

    std::uint64_t hash = 0;
    auto status = Encoding<std::uint64_t>::Read(&hash, reader);
    if (!status)
      return status;
    else if (hash != EntryListTraits<Table>::EntryList::Hash)
      return ErrorStatus::InvalidTableHash;

    SizeType count = 0;
    status = Encoding<SizeType>::Read(&count, reader);
    if (!status)
      return status;

    return ReadEntries(value, count, reader);
  }

 private:
  enum : std::size_t { Count = EntryListTraits<Table>::EntryList::Count };

  template <std::size_t Index>
  using PointerAt =
      typename EntryListTraits<Table>::EntryList::template At<Index>;

  static constexpr std::size_t ActiveEntryCount(const Table& /*value*/,
                                                Index<0>) {
    return 0;
  }

  template <std::size_t index>
  static constexpr std::size_t ActiveEntryCount(const Table& value,
                                                Index<index>) {
    using Pointer = PointerAt<index - 1>;
    const std::size_t count = Pointer::Resolve(value) ? 1 : 0;
    return ActiveEntryCount(value, Index<index - 1>{}) + count;
  }

  template <typename T, std::uint64_t Id>
  static constexpr std::size_t Size(const Entry<T, Id, ActiveEntry>& entry) {
    if (entry) {
      const std::size_t size = Encoding<T>::Size(entry.get());
      return Encoding<std::uint64_t>::Size(Id) +
             Encoding<std::uint64_t>::Size(size) + size;
    } else {
      return 0;
    }
  }

  template <typename T, std::uint64_t Id>
  static constexpr std::size_t Size(
      const Entry<T, Id, DeletedEntry>& /*entry*/) {
    return 0;
  }

  static constexpr std::size_t Size(const Table& /*value*/, Index<0>) {
    return 0;
  }

  template <std::size_t index>
  static constexpr std::size_t Size(const Table& value, Index<index>) {
    using Pointer = PointerAt<index - 1>;
    return Size(value, Index<index - 1>{}) + Size(Pointer::Resolve(value));
  }

  static void ClearEntries(Table* /*value*/, Index<0>) {}

  template <std::size_t index>
  static void ClearEntries(Table* value, Index<index>) {
    ClearEntries(value, Index<index - 1>{});
    PointerAt<index - 1>::Resolve(value)->clear();
  }

  template <typename T, std::uint64_t Id, typename Writer>
  static constexpr Status<void> WriteEntry(
      const Entry<T, Id, ActiveEntry>& entry, Writer* writer) {
    if (entry) {
      auto status = Encoding<std::uint64_t>::Write(Id, writer);
      if (!status)
        return status;

      const SizeType size = Encoding<T>::Size(entry.get());
      status = Encoding<SizeType>::Write(size, writer);
      if (!status)
        return status;

      // Use a BoundedWriter to track the number of bytes written. Since a few
      // encodings overestimate their size, the remaining bytes must be padded
      // out to match the size written above. This is a tradeoff that
      // potentially increases the encoding size to avoid unnecessary dynamic
      // memory allocation during encoding; some size savings could be made by
      // encoding the entry to a temporary buffer and then writing the exact
      // size for the binary container. However, overestimation is rare and
      // small, making the savings not worth the expense of the temporary
      // buffer.
      BoundedWriter<Writer> bounded_writer{writer, size};
      status = Encoding<T>::Write(entry.get(), &bounded_writer);
      if (!status)
        return status;

      return bounded_writer.WritePadding();
    } else {
      return {};
    }
  }

  template <typename T, std::uint64_t Id, typename Writer>
  static constexpr Status<void> WriteEntry(
      const Entry<T, Id, DeletedEntry>& /*entry*/, Writer* /*writer*/) {
    return {};
  }

  template <typename Writer>
  static constexpr Status<void> WriteEntries(const Table& /*value*/,
                                             Writer* /*writer*/, Index<0>) {
    return {};
  }

  template <std::size_t index, typename Writer>
  static constexpr Status<void> WriteEntries(const Table& value, Writer* writer,
                                             Index<index>) {
    auto status = WriteEntries(value, writer, Index<index - 1>{});
    if (!status)
      return status;

    using Pointer = PointerAt<index - 1>;
    return WriteEntry(Pointer::Resolve(value), writer);
  }

  template <typename T, std::uint64_t Id, typename Reader>
  static constexpr Status<void> ReadEntry(Entry<T, Id, ActiveEntry>* entry,
                                          Reader* reader) {
    // At the beginning of reading the table the destination entries are
    // cleared. If an entry is not cleared here then more than one entry for
    // the same id was written in violation of the table protocol.
    if (entry->empty()) {
      SizeType size = 0;
      auto status = Encoding<SizeType>::Read(&size, reader);
      if (!status)
        return status;

      // Default construct the entry;
      *entry = T{};

      // Use a BoundedReader to handle any padding that might follow the
      // value and catch invalid sizes while decoding inside the binary
      // container.
      BoundedReader<Reader> bounded_reader{reader, size};
      status = Encoding<T>::Read(&entry->get(), &bounded_reader);
      if (!status)
        return status;

      return bounded_reader.ReadPadding();
    } else {
      return ErrorStatus::DuplicateTableEntry;
    }
  }

  // Skips over the binary container for an entry.
  template <typename Reader>
  static constexpr Status<void> SkipEntry(Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    return reader->Skip(size);
  }

  template <typename T, std::uint64_t Id, typename Reader>
  static constexpr Status<void> ReadEntry(Entry<T, Id, DeletedEntry>* /*entry*/,
                                          Reader* reader) {
    return SkipEntry(reader);
  }

  template <typename Reader>
  static constexpr Status<void> ReadEntryForId(Table* /*value*/,
                                               std::uint64_t /*id*/,
                                               Reader* reader, Index<0>) {
    return SkipEntry(reader);
  }

  template <typename Reader, std::size_t index>
  static constexpr Status<void> ReadEntryForId(Table* value, std::uint64_t id,
                                               Reader* reader, Index<index>) {
    using Pointer = PointerAt<index - 1>;
    using Type = typename Pointer::Type;
    if (Type::Id == id)
      return ReadEntry(Pointer::Resolve(value), reader);
    else
      return ReadEntryForId(value, id, reader, Index<index - 1>{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadEntries(Table* value, SizeType count,
                                            Reader* reader) {
    for (SizeType i = 0; i < count; i++) {
      std::uint64_t id = 0;
      auto status = Encoding<std::uint64_t>::Read(&id, reader);
      if (!status)
        return status;

      status = ReadEntryForId(value, id, reader, Index<Count>{});
      if (!status)
        return status;
    }
    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_TABLE_H_




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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_VARIANT_H_
#define LIBNOP_INCLUDE_NOP_TYPES_VARIANT_H_

#include <cstdint>
#include <tuple>
#include <type_traits>

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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_
#define LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_

namespace nop {

// Type tag denoting an empty variant.
struct EmptyVariant {};

namespace detail {

// Type for matching tagged overloads.
template <typename T>
struct TypeTag {};

// Determines the type of the I-th element of Types....
template <std::size_t I, typename... Types>
using TypeForIndex = std::tuple_element_t<I, std::tuple<Types...>>;

// Determines the type tag for the I-th element of Types....
template <std::size_t I, typename... Types>
using TypeTagForIndex = TypeTag<TypeForIndex<I, Types...>>;

// Similar to std::is_constructible but evaluates to false for pointer to
// boolean construction: avoiding this conversion helps prevent subtle bugs in
// Variants with bool elements.
template <typename...>
struct IsConstructible;
template <typename T, typename U>
struct IsConstructible<T, U>
    : std::integral_constant<bool,
                             std::is_constructible<T, U>::value &&
                                 !(std::is_same<std::decay_t<T>, bool>::value &&
                                   std::is_pointer<std::decay_t<U>>::value)> {};
template <typename T, typename... Args>
struct IsConstructible<T, Args...> : std::is_constructible<T, Args...> {};

// Enable if T(Args...) is well formed.
template <typename R, typename T, typename... Args>
using EnableIfConstructible =
    typename std::enable_if<IsConstructible<T, Args...>::value, R>::type;
// Enable if T(Args...) is not well formed.
template <typename R, typename T, typename... Args>
using EnableIfNotConstructible =
    typename std::enable_if<!IsConstructible<T, Args...>::value, R>::type;

// Determines whether T is an element of Types...;
template <typename... Types>
struct HasType : std::false_type {};
template <typename T, typename U>
struct HasType<T, U> : std::is_same<std::decay_t<T>, std::decay_t<U>> {};
template <typename T, typename First, typename... Rest>
struct HasType<T, First, Rest...>
    : std::integral_constant<
          bool, HasType<T, First>::value || HasType<T, Rest...>::value> {};

// Defines set operations on a set of Types...
template <typename... Types>
struct Set {
  // Default specialization catches the empty set, which is always a subset.
  template <typename...>
  struct IsSubset : std::true_type {};
  template <typename T>
  struct IsSubset<T> : HasType<T, Types...> {};
  template <typename First, typename... Rest>
  struct IsSubset<First, Rest...>
      : std::integral_constant<
            bool, IsSubset<First>::value && IsSubset<Rest...>::value> {};
};

// Determines the number of elements of Types... that are constructible from
// From.
template <typename... Types>
struct ConstructibleCount;
template <typename From, typename To>
struct ConstructibleCount<From, To>
    : std::integral_constant<std::size_t, IsConstructible<To, From>::value> {};
template <typename From, typename First, typename... Rest>
struct ConstructibleCount<From, First, Rest...>
    : std::integral_constant<std::size_t,
                             IsConstructible<First, From>::value +
                                 ConstructibleCount<From, Rest...>::value> {};

// Enable if T is an element of Types...
template <typename R, typename T, typename... Types>
using EnableIfElement =
    typename std::enable_if<HasType<T, Types...>::value, R>::type;
// Enable if T is not an element of Types...
template <typename R, typename T, typename... Types>
using EnableIfNotElement =
    typename std::enable_if<!HasType<T, Types...>::value, R>::type;

// Enable if T is convertible to an element of Types... T is considered
// convertible IIF a single element of Types... is assignable from T and T is
// not a direct element of Types...
template <typename R, typename T, typename... Types>
using EnableIfConvertible =
    typename std::enable_if<!HasType<T, Types...>::value &&
                                ConstructibleCount<T, Types...>::value == 1,
                            R>::type;

// Enable if T is assignable to an element of Types... T is considered
// assignable IFF a single element of Types... is constructible from T or T is a
// direct element of Types.... Note that T is REQUIRED to be an element of
// Types... when multiple elements are constructible from T to prevent ambiguity
// in conversion.
template <typename R, typename T, typename... Types>
using EnableIfAssignable =
    typename std::enable_if<HasType<T, Types...>::value ||
                                ConstructibleCount<T, Types...>::value == 1,
                            R>::type;

// Selects a type for SFINAE constructor selection.
template <bool CondA, typename SelectA, typename SelectB>
using Select = std::conditional_t<CondA, SelectA, SelectB>;

// Recursive union type.
template <typename... Types>
union Union {};

// Specialization handling a singular type, terminating template recursion.
template <typename Type>
union Union<Type> {
  Union() {}
  ~Union() {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<Type>, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T, typename = EnableIfAssignable<void, T, Type>>
  Union(std::int32_t index, std::int32_t* index_out, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  Union(const Union& other, std::int32_t index) {
    if (index == 0)
      new (&first_) Type(other.first_);
  }
  Union(Union&& other, std::int32_t index) {
    if (index == 0)
      new (&first_) Type(std::move(other.first_));
  }
  Union(const Union&) = delete;
  Union(Union&&) = delete;
  void operator=(const Union&) = delete;
  void operator=(Union&&) = delete;

  Type& get(TypeTag<Type>) { return first_; }
  const Type& get(TypeTag<Type>) const { return first_; }
  EmptyVariant get(TypeTag<EmptyVariant>) const { return {}; }
  constexpr std::int32_t index(TypeTag<Type>) const { return 0; }

  template <typename... Args>
  std::int32_t Construct(TypeTag<Type>, Args&&... args) {
    new (&first_) Type(std::forward<Args>(args)...);
    return 0;
  }
  template <typename... Args>
  EnableIfConstructible<std::int32_t, Type, Args...> Construct(Args&&... args) {
    new (&first_) Type(std::forward<Args>(args)...);
    return 0;
  }

  void Destruct(std::int32_t target_index) {
    if (target_index == index(TypeTag<Type>{})) {
      (&get(TypeTag<Type>{}))->~Type();
    }
  }

  template <typename T>
  bool Assign(TypeTag<Type>, std::int32_t target_index, T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  EnableIfConstructible<bool, Type, T> Assign(std::int32_t target_index,
                                              T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  EnableIfNotConstructible<bool, Type, T> Assign(std::int32_t /*target_index*/,
                                                 T&& /*value*/) {
    return false;
  }

  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) {
    if (target_index == index(TypeTag<Type>{}))
      return std::forward<Op>(op)(get(TypeTag<Type>{}));
    else
      return std::forward<Op>(op)(get(TypeTag<EmptyVariant>{}));
  }
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) const {
    if (target_index == index(TypeTag<Type>{}))
      return std::forward<Op>(op)(get(TypeTag<Type>{}));
    else
      return std::forward<Op>(op)(get(TypeTag<EmptyVariant>{}));
  }

  template <typename... Args>
  bool Become(std::int32_t target_index, Args&&... args) {
    if (target_index == index(TypeTag<Type>{})) {
      Construct(TypeTag<Type>{}, std::forward<Args>(args)...);
      return true;
    } else {
      return false;
    }
  }

 private:
  Type first_;
};

// Specialization that recursively unions types from the paramater pack.
template <typename First, typename... Rest>
union Union<First, Rest...> {
  Union() {}
  ~Union() {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<First>, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T, typename U>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<T>, U&& value)
      : rest_(index + 1, index_out, TypeTag<T>{}, std::forward<U>(value)) {}
  Union(const Union& other, std::int32_t index) {
    if (index == 0)
      new (&first_) First(other.first_);
    else
      new (&rest_) Union<Rest...>(other.rest_, index - 1);
  }
  Union(Union&& other, std::int32_t index) {
    if (index == 0)
      new (&first_) First(std::move(other.first_));
    else
      new (&rest_) Union<Rest...>(std::move(other.rest_), index - 1);
  }
  Union(const Union&) = delete;
  Union(Union&&) = delete;
  void operator=(const Union&) = delete;
  void operator=(Union&&) = delete;

  struct FirstType {};
  struct RestType {};
  template <typename T>
  using SelectConstructor =
      Select<ConstructibleCount<T, First>::value == 1, FirstType, RestType>;

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value)
      : Union(index, index_out, std::forward<T>(value),
              SelectConstructor<T>{}) {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value, FirstType)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value, RestType)
      : rest_(index + 1, index_out, std::forward<T>(value)) {}

  First& get(TypeTag<First>) { return first_; }
  const First& get(TypeTag<First>) const { return first_; }
  constexpr std::int32_t index(TypeTag<First>) const { return 0; }

  template <typename T>
  T& get(TypeTag<T>) {
    return rest_.get(TypeTag<T>{});
  }
  template <typename T>
  const T& get(TypeTag<T>) const {
    return rest_.get(TypeTag<T>{});
  }
  template <typename T>
  constexpr std::int32_t index(TypeTag<T>) const {
    return 1 + rest_.index(TypeTag<T>{});
  }

  template <typename... Args>
  std::int32_t Construct(TypeTag<First>, Args&&... args) {
    new (&first_) First(std::forward<Args>(args)...);
    return 0;
  }
  template <typename T, typename... Args>
  std::int32_t Construct(TypeTag<T>, Args&&... args) {
    return 1 +
           rest_.Construct(TypeTag<T>{}, std::forward<Args>(args)...);
  }

  template <typename... Args>
  EnableIfConstructible<std::int32_t, First, Args...> Construct(
      Args&&... args) {
    new (&first_) First(std::forward<Args>(args)...);
    return 0;
  }
  template <typename... Args>
  EnableIfNotConstructible<std::int32_t, First, Args...> Construct(
      Args&&... args) {
    return 1 + rest_.Construct(std::forward<Args>(args)...);
  }

  void Destruct(std::int32_t target_index) {
    if (target_index == index(TypeTag<First>{})) {
      (get(TypeTag<First>{})).~First();
    } else {
      rest_.Destruct(target_index - 1);
    }
  }

  template <typename T>
  bool Assign(TypeTag<First>, std::int32_t target_index, T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T, typename U>
  bool Assign(TypeTag<T>, std::int32_t target_index, U&& value) {
    return rest_.Assign(TypeTag<T>{}, target_index - 1, std::forward<U>(value));
  }
  template <typename T>
  EnableIfConstructible<bool, First, T> Assign(std::int32_t target_index,
                                               T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return rest_.Assign(target_index - 1, std::forward<T>(value));
    }
  }
  template <typename T>
  EnableIfNotConstructible<bool, First, T> Assign(std::int32_t target_index,
                                                  T&& value) {
    return rest_.Assign(target_index - 1, std::forward<T>(value));
  }

  // Recursively traverses the union and calls Op on the active value when the
  // active type is found. If the union is empty Op is called on EmptyVariant.
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) {
    if (target_index == index(TypeTag<First>{}))
      return std::forward<Op>(op)(get(TypeTag<First>{}));
    else
      return rest_.Visit(target_index - 1, std::forward<Op>(op));
  }
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) const {
    if (target_index == index(TypeTag<First>{}))
      return std::forward<Op>(op)(get(TypeTag<First>{}));
    else
      return rest_.Visit(target_index - 1, std::forward<Op>(op));
  }

  template <typename... Args>
  bool Become(std::int32_t target_index, Args&&... args) {
    if (target_index == index(TypeTag<First>{})) {
      Construct(TypeTag<First>{}, std::forward<Args>(args)...);
      return true;
    } else {
      return rest_.Become(target_index - 1, std::forward<Args>(args)...);
    }
  }

 private:
  First first_;
  Union<Rest...> rest_;
};

}  // namespace detail
}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_


namespace nop {

//
// Variant type that can store one of a number of types.
//
// Variant is a type-safe union that can store any one of the types it is
// instantiated with. A Variant may only hold one type at a time and supports
// examination of which type is currently stored and various means of
// manipulating the stored value. In particular, Variant supports generic lambda
// visitor functions, enabling flexible value manipulation.
//
// Variant has the notion of emptiness, or holding none of the types it is
// instantiated with. This means Variant is default constructible to empty,
// regardless of whether any of its element types are default constructible.
//

template <typename... Types>
class Variant {
 private:
  // Convenience types.
  template <typename T>
  using TypeTag = detail::TypeTag<T>;
  template <typename T>
  using DecayedTypeTag = TypeTag<std::decay_t<T>>;
  template <std::size_t I>
  using TypeForIndex = detail::TypeForIndex<I, Types...>;
  template <std::size_t I>
  using TypeTagForIndex = detail::TypeTagForIndex<I, Types...>;
  template <typename T>
  using HasType = detail::HasType<T, Types...>;
  template <typename R, typename T>
  using EnableIfElement = detail::EnableIfElement<R, T, Types...>;
  template <typename R, typename T>
  using EnableIfConvertible = detail::EnableIfConvertible<R, T, Types...>;
  template <typename R, typename T>
  using EnableIfAssignable = detail::EnableIfAssignable<R, T, Types...>;

  struct Direct {};
  struct Convert {};
  template <typename T>
  using SelectConstructor = detail::Select<HasType<T>::value, Direct, Convert>;

  // Constructs by type tag when T is an direct element of Types...
  template <typename T>
  explicit Variant(T&& value, Direct)
      : value_(0, &index_, DecayedTypeTag<T>{}, std::forward<T>(value)) {}
  // Conversion constructor when T is not a direct element of Types...
  template <typename T>
  explicit Variant(T&& value, Convert)
      : value_(0, &index_, std::forward<T>(value)) {}

 public:
  // Variants are default construcible, regardless of whether the elements are
  // default constructible. Default consruction yields an empty Variant.
  Variant() {}
  explicit Variant(EmptyVariant value) { Construct(value); }
  ~Variant() { Destruct(); }

  Variant(const Variant& other)
      : index_{other.index_}, value_{other.value_, other.index_} {}
  Variant(Variant&& other)
      : index_{other.index_}, value_{std::move(other.value_), other.index_} {}

  // Copy and move construction from Variant types. Each element of OtherTypes
  // must be convertible to an element of Types.
  template <typename... OtherTypes>
  explicit Variant(const Variant<OtherTypes...>& other) {
    other.Visit([this](const auto& value) { this->Construct(value); });
  }
  template <typename... OtherTypes>
  explicit Variant(Variant<OtherTypes...>&& other) {
    other.Visit([this](auto&& value) { this->Construct(std::move(value)); });
  }

  Variant& operator=(const Variant& other) {
    other.Visit([this](const auto& value) { *this = value; });
    return *this;
  }
  Variant& operator=(Variant&& other) {
    other.Visit([this](auto&& value) { *this = std::move(value); });
    return *this;
  }

  // Construction from non-Variant types.
  template <typename T, typename = EnableIfAssignable<void, T>>
  explicit Variant(T&& value)
      : Variant(std::forward<T>(value), SelectConstructor<T>{}) {}

  // Performs assignment from type T belonging to Types. This overload takes
  // priority to prevent implicit conversion in cases where T is implicitly
  // convertible to multiple elements of Types.
  template <typename T>
  EnableIfElement<Variant&, T> operator=(T&& value) {
    Assign(DecayedTypeTag<T>{}, std::forward<T>(value));
    return *this;
  }

  // Performs assignment from type T not belonging to Types. This overload
  // matches in cases where conversion is the only viable option.
  template <typename T>
  EnableIfConvertible<Variant&, T> operator=(T&& value) {
    Assign(std::forward<T>(value));
    return *this;
  }

  // Handles assignment from the empty type. This overload supports assignment
  // in visitors using generic lambdas.
  Variant& operator=(EmptyVariant) {
    Destruct();
    return *this;
  }

  // Assignment from Variant types. Each element of OtherTypes must be
  // convertible to an element of Types. Forwards through non-Variant assignment
  // operators to apply conversion checks.
  template <typename... OtherTypes>
  Variant& operator=(const Variant<OtherTypes...>& other) {
    other.Visit([this](const auto& value) { *this = value; });
    return *this;
  }
  template <typename... OtherTypes>
  Variant& operator=(Variant<OtherTypes...>&& other) {
    other.Visit([this](auto&& value) { *this = std::move(value); });
    return *this;
  }

  // Becomes the target type, constructing a new element from the given
  // arguments if necessary. No action is taken if the active element is already
  // the target type. Otherwise the active element is destroyed and replaced by
  // constructing an element of the new type using |Args|. An invalid target
  // type index results in an empty Variant.
  template <typename... Args>
  void Become(std::int32_t target_index, Args&&... args) {
    if (target_index != index()) {
      Destruct();
      index_ = value_.Become(target_index, std::forward<Args>(args)...)
                   ? target_index
                   : kEmptyIndex;
    }
  }

  // Invokes |Op| on the active element. If the Variant is empty |Op| is invoked
  // on EmptyVariant.
  template <typename Op>
  decltype(auto) Visit(Op&& op) {
    return value_.Visit(index_, std::forward<Op>(op));
  }
  template <typename Op>
  decltype(auto) Visit(Op&& op) const {
    return value_.Visit(index_, std::forward<Op>(op));
  }

  // Index returned when the Variant is empty.
  enum : std::int32_t { kEmptyIndex = -1 };

  // Returns the index of the given type.
  template <typename T>
  constexpr std::int32_t index_of() const {
    static_assert(HasType<T>::value, "T is not an element type of Variant.");
    return value_.index(DecayedTypeTag<T>{});
  }

  // Returns the index of the active type. If the Variant is empty -1 is
  // returned.
  std::int32_t index() const { return index_; }

  // Returns true if the given type is active, false otherwise.
  template <typename T>
  bool is() const {
    static_assert(HasType<T>::value, "T is not an element type of Variant.");
    return index() == index_of<T>();
  }

  // Returns true if the Variant is empty, false otherwise.
  bool empty() const { return index() == kEmptyIndex; }

  // Element accessors. Returns a pointer to the active value if the given
  // type/index is active, otherwise nullptr is returned.
  template <typename T>
  T* get() {
    if (is<T>())
      return &value_.get(DecayedTypeTag<T>{});
    else
      return nullptr;
  }
  template <typename T>
  const T* get() const {
    if (is<T>())
      return &value_.get(DecayedTypeTag<T>{});
    else
      return nullptr;
  }
  template <std::size_t I>
  TypeForIndex<I>* get() {
    if (is<TypeForIndex<I>>())
      return &value_.get(TypeTagForIndex<I>{});
    else
      return nullptr;
  }
  template <std::size_t I>
  const TypeForIndex<I>* get() const {
    if (is<TypeForIndex<I>>())
      return &value_.get(TypeTagForIndex<I>{});
    else
      return nullptr;
  }

 private:
  std::int32_t index_ = kEmptyIndex;
  detail::Union<std::decay_t<Types>...> value_;

  // Constructs an element from the given arguments and sets the Variant to the
  // resulting type.
  template <typename... Args>
  void Construct(Args&&... args) {
    index_ = value_.template Construct(std::forward<Args>(args)...);
  }
  void Construct(EmptyVariant) {}

  // Destroys the active element of the Variant.
  void Destruct() {
    value_.Destruct(index_);
    index_ = kEmptyIndex;
  }

  // Assigns the Variant when non-empty and the current type matches the target
  // type, otherwise destroys the current value and constructs a element of the
  // new type. Tagged assignment is used when T is an element of the Variant to
  // prevent implicit conversion in cases where T is implicitly convertible to
  // multiple element types.
  template <typename T, typename U>
  void Assign(TypeTag<T>, U&& value) {
    if (!value_.template Assign(TypeTag<T>{}, index_, std::forward<U>(value))) {
      Destruct();
      Construct(TypeTag<T>{}, std::forward<U>(value));
    }
  }
  template <typename T>
  void Assign(T&& value) {
    if (!value_.template Assign(index_, std::forward<T>(value))) {
      Destruct();
      Construct(std::forward<T>(value));
    }
  }
};

// Utility type to extract/convert values from a variant. This class simplifies
// conditional logic to get/move/swap/action values from a variant when one or
// more elements are compatible with the destination type.
//
// Example:
//    Variant<int, bool, std::string> v(10);
//    bool bool_value;
//    if (IfAnyOf<int, bool>::Get(v, &bool_value)) {
//      DoSomething(bool_value);
//    } else {
//      HandleInvalidType();
//    }
//    IfAnyOf<int>::Call(v, [](const auto& value) { DoSomething(value); });
//
template <typename... ValidTypes>
struct IfAnyOf {
  // Calls Op on the underlying value of the variant and returns true when the
  // variant is a valid type, otherwise does nothing and returns false.
  template <typename Op, typename... Types>
  static bool Call(Variant<Types...>* variant, Op&& op) {
    static_assert(
        detail::Set<Types...>::template IsSubset<ValidTypes...>::value,
        "ValidTypes may only contain element types from the Variant.");
    return variant->Visit(CallOp<Op>{std::forward<Op>(op)});
  }
  template <typename Op, typename... Types>
  static bool Call(const Variant<Types...>* variant, Op&& op) {
    static_assert(
        detail::Set<Types...>::template IsSubset<ValidTypes...>::value,
        "ValidTypes may only contain element types from the Variant.");
    return variant->Visit(CallOp<Op>{std::forward<Op>(op)});
  }

  // Gets/converts the underlying value of the variant to type T and returns
  // true when the variant is a valid type, otherwise does nothing and returns
  // false.
  template <typename T, typename... Types>
  static bool Get(const Variant<Types...>* variant, T* value_out) {
    return Call(variant,
                [value_out](const auto& value) { *value_out = value; });
  }

  // Moves the underlying value of the variant and returns true when the variant
  // is a valid type, otherwise does nothing and returns false.
  template <typename T, typename... Types>
  static bool Take(Variant<Types...>* variant, T* value_out) {
    return Call(variant,
                [value_out](auto&& value) { *value_out = std::move(value); });
  }

  // Swaps the underlying value of the variant with |*value_out| and returns
  // true when the variant is a valid type, otherwise does nothing and returns
  // false.
  template <typename T, typename... Types>
  static bool Swap(Variant<Types...>* variant, T* value_out) {
    return Call(variant,
                [value_out](auto&& value) { std::swap(*value_out, value); });
  }

 private:
  template <typename Op>
  struct CallOp {
    Op&& op;
    template <typename U>
    detail::EnableIfNotElement<bool, U, ValidTypes...> operator()(U&&) {
      return false;
    }
    template <typename U>
    detail::EnableIfElement<bool, U, ValidTypes...> operator()(const U& value) {
      std::forward<Op>(op)(value);
      return true;
    }
    template <typename U>
    detail::EnableIfElement<bool, U, ValidTypes...> operator()(U&& value) {
      std::forward<Op>(op)(std::forward<U>(value));
      return true;
    }
  };
};

}  // namespace nop

// Overloads of std::get<T> and std::get<I> for nop::Variant.
namespace std {

template <typename T, typename... Types>
inline T& get(::nop::Variant<Types...>& v) {
  return *v.template get<T>();
}
template <typename T, typename... Types>
inline T&& get(::nop::Variant<Types...>&& v) {
  return std::move(*v.template get<T>());
}
template <typename T, typename... Types>
inline const T& get(const ::nop::Variant<Types...>& v) {
  return *v.template get<T>();
}
template <std::size_t I, typename... Types>
inline ::nop::detail::TypeForIndex<I, Types...>& get(
    ::nop::Variant<Types...>& v) {
  return *v.template get<I>();
}
template <std::size_t I, typename... Types>
inline ::nop::detail::TypeForIndex<I, Types...>&& get(
    ::nop::Variant<Types...>&& v) {
  return std::move(*v.template get<I>());
}
template <std::size_t I, typename... Types>
inline const ::nop::detail::TypeForIndex<I, Types...>& get(
    const ::nop::Variant<Types...>& v) {
  return *v.template get<I>();
}

}  // namespace std

#endif  // LIBNOP_INCLUDE_NOP_TYPES_VARIANT_H_


// This header defines rules for which types have equivalent encodings. Types
// with equivalent encodings my be legally substituted during serialization and
// interface binding to enable greater flexibility in user code.
//
// For example, std::vector<T> and std::array<T> of the same type T encode using
// the same format.

namespace nop {

// Compares A and B for fungibility. This base type determines convertibility by
// equivalence of the underlying types of A and B. Specializations of this type
// handle the rules for which complex types are fungible.
template <typename A, typename B, typename Enabled = void>
struct IsFungible : std::is_same<std::decay_t<A>, std::decay_t<B>> {};

// Enable if A and B are fungible.
template <typename A, typename B, typename Return = void>
using EnableIfFungible =
    typename std::enable_if<IsFungible<A, B>::value, Return>::type;

// Compares two function types to see if the return types and arguments are
// fungible.
template <typename ReturnA, typename ReturnB, typename... ArgsA,
          typename... ArgsB>
struct IsFungible<ReturnA(ArgsA...), ReturnB(ArgsB...),
                  std::enable_if_t<sizeof...(ArgsA) == sizeof...(ArgsB)>>
    : And<IsFungible<std::decay_t<ReturnA>, std::decay_t<ReturnB>>,
          IsFungible<std::decay_t<ArgsA>, std::decay_t<ArgsB>>...> {};

// Compares two std::arrays to see if the element types are fungible.
template <typename A, typename B, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares two C arrays to see if the element types are fungible. Sizes are
// explicitly compared to avoid falling back on the base IsFungible type which
// would decay A and B to pointers and fail to compare different array sizes
// correctly.
template <typename A, typename B, std::size_t SizeA, std::size_t SizeB>
struct IsFungible<A[SizeA], B[SizeB]>
    : And<IsFungible<std::decay_t<A>, std::decay_t<B>>,
          std::integral_constant<bool, SizeA == SizeB>> {};

// Compares two std::vectors to see if the element types are fungible.
template <typename A, typename B, typename AllocatorA, typename AllocatorB>
struct IsFungible<std::vector<A, AllocatorA>, std::vector<B, AllocatorB>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares two std::maps to see if the element types are fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::map<KeyA, ValueA, AnyA...>,
                  std::map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares two std::unordered_maps to see if the element types are fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::unordered_map<KeyA, ValueA, AnyA...>,
                  std::unordered_map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares a std::map and a std::unordered_map to see if the element types are
// fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::map<KeyA, ValueA, AnyA...>,
                  std::unordered_map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::unordered_map<KeyA, ValueA, AnyA...>,
                  std::map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares two std::tuples to see if the corresponding elements are
// fungible. Fungible tuples must have the same number of elements.
template <typename... A, typename... B>
struct IsFungible<std::tuple<A...>, std::tuple<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<B>>...> {};
template <typename... A, typename... B>
struct IsFungible<std::tuple<A...>, std::tuple<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares two std::pairs to see if the corresponding elements are fungible.
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::pair<A, B>, std::pair<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};

// Compares std::pair with a two-element std::tuple to see if the corresponding
// elements are fungible.
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::pair<A, B>, std::tuple<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::tuple<A, B>, std::pair<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};

// Compares std::vector with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral vector element type.
template <typename T, typename Allocator, typename... Ts>
struct IsFungible<std::vector<T, Allocator>, std::tuple<Ts...>,
                  EnableIfNotIntegral<T>> : And<IsFungible<T, Ts>...> {};
template <typename T, typename Allocator, typename... Ts>
struct IsFungible<std::tuple<Ts...>, std::vector<T, Allocator>,
                  EnableIfNotIntegral<T>> : And<IsFungible<T, Ts>...> {};

// Compares std::array with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral array element type.
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::array<T, Size>, std::tuple<Ts...>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::tuple<Ts...>, std::array<T, Size>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};

// Compares C array with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral C array element type.
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    T[Size], std::tuple<Ts...>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::tuple<Ts...>, T[Size],
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};

// Compares std::vector and std::array to see if the element types are
// fungible.
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::vector<A, Allocator>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::vector<B, Allocator>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares C array and std::vector to see if the elements types are fungible.
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<A[Size], std::vector<B, Allocator>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::vector<A, Allocator>, B[Size]>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares C array and std::array to see if the element types are fungible.
template <typename A, typename B, std::size_t Size>
struct IsFungible<A[Size], std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, std::size_t Size>
struct IsFungible<std::array<A, Size>, B[Size]>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Result<ErrorEnum, A> and Result<ErrorEnum, B> to see if A and B are
// fungible. ErrorEnum must be the same between fungible Result types because
// different enum types are not fungible.
template <typename ErrorEnum, typename A, typename B>
struct IsFungible<Result<ErrorEnum, A>, Result<ErrorEnum, B>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Optional<A> and Optional<B> to see if A and B are fungible.
template <typename A, typename B>
struct IsFungible<Optional<A>, Optional<B>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Entry<A> and Entry<B> to see if A and B are fungible.
template <typename A, typename B, std::uint64_t Id, typename Type>
struct IsFungible<Entry<A, Id, Type>, Entry<B, Id, Type>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Variant<A...> and Variant<B...> to see if every member of A is
// fungible with every corresponding member of B.
template <typename... A, typename... B>
struct IsFungible<Variant<A...>, Variant<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<A, B>...> {};
template <typename... A, typename... B>
struct IsFungible<Variant<A...>, Variant<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares two LogicalBuffers to see if they are fungible.
template <typename A, typename B, typename SizeTypeA, typename SizeTypeB,
          bool IsUnboundedA, bool IsUnboundedB, typename EnabledA,
          typename EnabledB>
struct IsFungible<LogicalBuffer<A, SizeTypeA, IsUnboundedA, EnabledA>,
                  LogicalBuffer<B, SizeTypeB, IsUnboundedB, EnabledB>>
    : IsFungible<A, B> {};

// Compares a std::vector and a LogicalBuffer to see if they are fungible.
template <typename A, typename B, typename AllocatorA, typename SizeTypeB,
          bool IsUnboundedB, typename EnabledB>
struct IsFungible<std::vector<A, AllocatorA>,
                  LogicalBuffer<B, SizeTypeB, IsUnboundedB, EnabledB>>
    : IsFungible<std::vector<A, AllocatorA>, B> {};
template <typename A, typename B, typename SizeTypeA, bool IsUnboundedA,
          typename EnabledA, typename AllocatorB>
struct IsFungible<LogicalBuffer<A, SizeTypeA, IsUnboundedA, EnabledA>,
                  std::vector<B, AllocatorB>>
    : IsFungible<A, std::vector<B, AllocatorB>> {};

// Compares MemberList<A...> and MemberList<B...> to see if every
// MemberPointer::Type in A is fungible with every MemberPointer::Type in B.
template <typename... A, typename... B>
struct IsFungible<MemberList<A...>, MemberList<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<typename A::Type, typename B::Type>...> {};
template <typename... A, typename... B>
struct IsFungible<MemberList<A...>, MemberList<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares user-defined types A and B to see if every member is fungible.
template <typename A, typename B>
struct IsFungible<
    A, B, std::enable_if_t<HasMemberList<A>::value && HasMemberList<B>::value>>
    : IsFungible<typename MemberListTraits<A>::MemberList,
                 typename MemberListTraits<B>::MemberList> {};

// Compares user-defined value wrapper types A and B to see if the values are
// fungible.
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<IsValueWrapper<A>::value && IsValueWrapper<B>::value>>
    : IsFungible<typename ValueWrapperTraits<A>::MemberList,
                 typename ValueWrapperTraits<B>::MemberList> {};

// Compares user-defined value wrapper type A and non-wrapper type B and vice
// versa to see if the wrapped and non-wrapped types are fungible.
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<IsValueWrapper<A>::value && !IsValueWrapper<B>::value>>
    : IsFungible<typename ValueWrapperTraits<A>::Pointer::Type, B> {};
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<!IsValueWrapper<A>::value && IsValueWrapper<B>::value>>
    : IsFungible<A, typename ValueWrapperTraits<B>::Pointer::Type> {};

template <typename Hash, typename... A, typename... B>
struct IsFungible<EntryList<Hash, A...>, EntryList<Hash, B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<typename A::Type, typename B::Type>...> {};
template <typename HashA, typename HashB, typename... A, typename... B>
struct IsFungible<EntryList<HashA, A...>, EntryList<HashB, B...>,
                  std::enable_if_t<!std::is_same<HashA, HashB>::value ||
                                   sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares table types A and B to see if every member is fungible.
template <typename A, typename B>
struct IsFungible<
    A, B, std::enable_if_t<HasEntryList<A>::value && HasEntryList<B>::value>>
    : IsFungible<typename EntryListTraits<A>::EntryList,
                 typename EntryListTraits<B>::EntryList> {};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_


namespace nop {

// Implements a simple compile-time type-based protocol check. Overload
// resolution for Write/Read methods succeeds if the argument passed for
// serialization/deserialization is compatible with the protocol type
// ProtocolType.
template <typename ProtocolType>
struct Protocol {
  template <typename Serializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static constexpr Status<void> Write(Serializer* serializer, const T& value) {
    return serializer->Write(value);
  }

  template <typename Deserializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static constexpr Status<void> Read(Deserializer* deserializer, T* value) {
    return deserializer->Read(value);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_PROTOCOL_H_
/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_VALUE_H_
#define LIBNOP_INCLUDE_NOP_VALUE_H_




namespace nop {

//
// Value wrappers are structs or classes that have been annotated so that the
// serialization engine understands how to read and write them. They are similar
// to user-defined structures except that only a single member or logical buffer
// pair may be specified. When serialized and deserailized, value wrappers only
// take the wire format of their underlying type, avoiding any additional
// overhead. This is primarily useful for wrapping a bit of constructor,
// operator, and accessor logic around another type.
//
// Annotation is performed by invoking the NOP_VALUE(type, member) macro in the
// body of the struct, class, or template.
//
// Example of a simple float to/from fixed point wrapper template type:
//
//   template <typename Integer, std::size_t FractionalBits_>
//   class Fixed {
//    public:
//     enum : std::size_t {
//       Bits = sizeof(Integer) * 8,
//       FractionalBits = FractionalBits_,
//       IntegralBits = Bits - FractionalBits
//     };
//     enum : std::size_t { Power = 1 << FractionalBits };
//
//     static_assert(std::is_integral<Integer>::value, "");
//     static_assert(FractionalBits < Bits, "");
//
//     constexpr Fixed() = default;
//     constexpr Fixed(const Fixed&) = default;
//     constexpr Fixed(float f) { value_ = std::round(f * Power); }
//
//     Fixed& operator=(const Fixed&) = default;
//     Fixed& operator=(float f) { value_ = std::round(f * Power); }
//
//     constexpr float float_value() const {
//       return static_cast<float>(value_) / Power;
//     }
//
//     explicit constexpr operator float() const { return float_value(); }
//     constexpr Integer value() const { return value_; }
//
//    private:
//     Integer value_;
//     NOP_VALUE(Fixed, value_);
//   };
//

// Defines the member or logical buffer pair to use as the value and effective
// type during serialization/deserailization. This macro must be invoked once
// within the struct/calss definition, preferrably in the private section for
// classes with private data.
#define NOP_VALUE(type, member)            \
  template <typename, typename>            \
  friend struct ::nop::Encoding;           \
  template <typename, typename>            \
  friend struct ::nop::IsValueWrapper;     \
  template <typename, typename>            \
  friend struct ::nop::ValueWrapperTraits; \
  using NOP__VALUE = ::nop::MemberList<_NOP_VALUE_MEMBER(type, member)>

//
// Utility macros used by the macro above.
//

// Generates a pair of template arguments (member pointer type and value) to be
// passed to MemberPointer<MemberPointerType, MemberPointerValue, ...> from the
// given type name and member name.
#define _NOP_VALUE_MEMBER_POINTER(type, member) \
  decltype(&type::member), &type::member

// Generates a MemberPointer type definition, given a type name and a variable
// number of member names. The first member name is handled here, while the
// remaining member names are passed to _NOP_VALUE_MEMBER_POINTER_NEXT for
// recursive expansion.
#define _NOP_VALUE_MEMBER_POINTER_FIRST(type, ...)                 \
  ::nop::MemberPointer<                                            \
      _NOP_VALUE_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
          _NOP_VALUE_MEMBER_POINTER_NEXT(type, _NOP_REST_ARG(__VA_ARGS__))>

// Recursively handles the remaining member names in the template argument list
// for MemberPointer.
#define _NOP_VALUE_MEMBER_POINTER_NEXT(type, ...)                 \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__))                        \
  (, _NOP_VALUE_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
         _NOP_DEFER2(__NOP_VALUE_MEMBER_POINTER_NEXT)()(          \
             type, _NOP_REST_ARG(__VA_ARGS__)))(/*done*/)

// Indirection to enable recursive macro expansion of
// _NOP_VALUE_MEMBER_POINTER_NEXT.
#define __NOP_VALUE_MEMBER_POINTER_NEXT() _NOP_VALUE_MEMBER_POINTER_NEXT

// Defines a single MemberPointer type given a type and a member name or group
// of member names.
#define _NOP_VALUE_MEMBER(type, member) \
  NOP_MAP_ARGS(_NOP_VALUE_MEMBER_POINTER_FIRST, (type), member)

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_VALUE_H_
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

#ifndef LIBNOP_INCLUDE_NOP_SERIALIZER_H_
#define LIBNOP_INCLUDE_NOP_SERIALIZER_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_
#define LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_

#include <array>




namespace nop {

//
// std::array<T, N> and T[N] encoding format for non-integral types:
//
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements must be valid encodings of type T.
//
// std::array<T, N> and T[N] encoding format for integral types:
//
// +-----+---------+---//----+
// | BIN | INT64:L | L BYTES |
// +-----+---------+---//----+
//
// Where L = N * sizeof(T).
//
// Elements are stored as direct little-endian representation of the integral
// value, each element is sizeof(T) bytes in size.
//

template <typename T, std::size_t Length>
struct Encoding<std::array<T, Length>, EnableIfNotIntegral<T>>
    : EncodingIO<std::array<T, Length>> {
  using Type = std::array<T, Length>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    std::size_t element_size_sum = 0;
    for (std::size_t i=0; i < Length; i++)
      element_size_sum += Encoding<T>::Size(value[i]);

    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(Length) +
           element_size_sum;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(Length, writer);
    if (!status)
      return status;

    for (SizeType i = 0; i < Length; i++) {
      status = Encoding<T>::Write(value[i], writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus::InvalidContainerLength;

    for (SizeType i = 0; i < Length; i++) {
      status = Encoding<T>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    return {};
  }
};

template <typename T, std::size_t Length>
struct Encoding<T[Length], EnableIfNotIntegral<T>> : EncodingIO<T[Length]> {
  using Type = T[Length];

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    std::size_t element_size_sum = 0;
    for (std::size_t i=0; i < Length; i++)
      element_size_sum += Encoding<T>::Size(value[i]);

    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(Length) +
           element_size_sum;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(Length, writer);
    if (!status)
      return status;

    for (SizeType i = 0; i < Length; i++) {
      status = Encoding<T>::Write(value[i], writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus::InvalidContainerLength;

    for (SizeType i = 0; i < Length; i++) {
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

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = sizeof(T) * Length;
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(size) +
           size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(Length * sizeof(T), writer);
    if (!status)
      return status;

    return writer->Write(value.begin(), value.end());
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length * sizeof(T))
      return ErrorStatus::InvalidContainerLength;

    return reader->Read(&(*value)[0], &(*value)[Length]);
  }
};

template <typename T, std::size_t Length>
struct Encoding<T[Length], EnableIfIntegral<T>> : EncodingIO<T[Length]> {
  using Type = T[Length];

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = Length * sizeof(T);
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(size) +
           size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(Length * sizeof(T), writer);
    if (!status)
      return status;

    return writer->Write(&value[0], &value[Length]);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length * sizeof(T))
      return ErrorStatus::InvalidContainerLength;

    return reader->Read(&(*value)[0], &(*value)[Length]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_


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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ENUM_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENUM_H_

#include <type_traits>



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
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const T& value, Writer* writer) {
    return Encoding<IntegerType>::WritePayload(
        prefix, reinterpret_cast<const IntegerType&>(value), writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, T* value,
                                            Reader* reader) {
    return Encoding<IntegerType>::ReadPayload(
        prefix, reinterpret_cast<IntegerType*>(value), reader);
  }

 private:
  using IntegerType = typename std::underlying_type<T>::type;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENUM_H_


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

#ifndef LIBNOP_INCLUDE_NOP_BASE_MAP_H_
#define LIBNOP_INCLUDE_NOP_BASE_MAP_H_

#include <map>
#include <numeric>
#include <unordered_map>



namespace nop {

//
// std::map<Key, T> and std::unordered_map<Key, T> encoding format:
//
// +-----+---------+--------//---------+
// | MAP | INT64:N | N KEY/VALUE PAIRS |
// +-----+---------+--------//---------+
//
// Each pair must be a valid encoding of Key followed by a valid encoding of T.
//

template <typename Key, typename T, typename Compare, typename Allocator>
struct Encoding<std::map<Key, T, Compare, Allocator>>
    : EncodingIO<std::map<Key, T, Compare, Allocator>> {
  using Type = std::map<Key, T, Compare, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    value->clear();
    for (SizeType i = 0; i < size; i++) {
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

template <typename Key, typename T, typename Hash, typename KeyEqual,
          typename Allocator>
struct Encoding<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>>
    : EncodingIO<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> {
  using Type = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    value->clear();
    for (SizeType i = 0; i < size; i++) {
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MAP_H_


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

#ifndef LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_




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
  static constexpr Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    if (value)
      return Encoding<T>::WritePayload(prefix, value.get(), writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, Type* value,
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

#ifndef LIBNOP_INCLUDE_NOP_BASE_PAIR_H_
#define LIBNOP_INCLUDE_NOP_BASE_PAIR_H_

#include <type_traits>
#include <utility>



namespace nop {

//
// std::pair<T, U> encoding format:
//
// +-----+---------+-------+--------+
// | ARY | INT64:2 | FIRST | SECOND |
// +-----+---------+-------+--------+
//
// First must be a valid encoding of T; second must be a valid encoding of U.
//

template <typename T, typename U>
struct Encoding<std::pair<T, U>> : EncodingIO<std::pair<T, U>> {
  using Type = std::pair<T, U>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(2u) +
           Encoding<First>::Size(value.first) +
           Encoding<Second>::Size(value.second);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(2u, writer);
    if (!status)
      return status;

    status = Encoding<First>::Write(value.first, writer);
    if (!status)
      return status;

    return Encoding<Second>::Write(value.second, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != 2u)
      return ErrorStatus::InvalidContainerLength;

    status = Encoding<First>::Read(&value->first, reader);
    if (!status)
      return status;

    return Encoding<Second>::Read(&value->second, reader);
  }

 private:
  using First = std::remove_cv_t<std::remove_reference_t<T>>;
  using Second = std::remove_cv_t<std::remove_reference_t<U>>;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_PAIR_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_
#define LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_

#include <functional>



namespace nop {

// std::reference_wrapper<T> encoding forwards to Encoding<T>.

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
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const Type& value,
                                             Writer* writer) {
    return Encoding<T>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                            Reader* reader) {
    return Encoding<T>::ReadPayload(prefix, &value->get(), reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
#define LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_

#include <memory>




namespace nop {

//
// Serializer and Deserializer template types provide the basic interface for
// writing and reading C++ types to a writer or reader class. There are several
// types of specializations of Serializer and Deserializer: those that contain
// an internal instance of the writer or reader and those that wrap a pointer or
// unique pointer to an external writer or reader.
//
// Example of instantiating a Serializer with an internal Writer:
//
//   Serializer<StreamWriter<std::stringstream>> serializer;
//   auto status = serializer.Write(data_type);
//
// Example of instantiating a Serializer with an external Writer:
//
//   using Writer = StreamWriter<std::stringstream>;
//   Writer stream_writer;
//   Serializer<Writer> serializer{&stream_writer};
//   auto status = serializer.Write(data_type);
//
// Which specialization to use depends on the situation and whether the writer
// or reader will be used in different contexts or only for serialization /
// deserialization tasks.
//

// Implementation of Write method common to all Serializer specializations.
struct SerializerCommon {
  template <typename T, typename Writer>
  static constexpr Status<void> Write(const T& value, Writer* writer) {
    // Determine how much space to prepare the writer for.
    const std::size_t size_bytes = Encoding<T>::Size(value);

    // Prepare the writer for the serialized data.
    auto status = writer->Prepare(size_bytes);
    if (!status)
      return status;

    // Serialize the data to the writer.
    return Encoding<T>::Write(value, writer);
  }
};

// Serializer with internal instance of Writer.
template <typename Writer>
class Serializer {
 public:
  template <typename... Args>
  constexpr Serializer(Args&&... args) : writer_{std::forward<Args>(args)...} {}

  constexpr Serializer(Serializer&&) = default;
  constexpr Serializer& operator=(Serializer&&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, &writer_);
  }

  constexpr const Writer& writer() const { return writer_; }
  constexpr Writer& writer() { return writer_; }
  constexpr Writer&& take() { return std::move(writer_); }

 private:
  Writer writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

// Serializer that wraps a pointer to Writer.
template <typename Writer>
class Serializer<Writer*> {
 public:
  constexpr Serializer() : writer_{nullptr} {}
  constexpr Serializer(Writer* writer) : writer_{writer} {}
  constexpr Serializer(const Serializer&) = default;
  constexpr Serializer& operator=(const Serializer&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, writer_);
  }

  constexpr const Writer& writer() const { return *writer_; }
  constexpr Writer& writer() { return *writer_; }

 private:
  Writer* writer_;
};

// Serializer that wraps a unique pointer to Writer.
template <typename Writer>
class Serializer<std::unique_ptr<Writer>> {
 public:
  constexpr Serializer() = default;
  constexpr Serializer(std::unique_ptr<Writer> writer)
      : writer_{std::move(writer)} {}
  constexpr Serializer(Serializer&&) = default;
  constexpr Serializer& operator=(Serializer&&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, writer_.get());
  }

  constexpr const Writer& writer() const { return *writer_; }
  constexpr Writer& writer() { return *writer_; }

 private:
  std::unique_ptr<Writer> writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

// Deserializer that wraps an internal instance of Reader.
template <typename Reader>
class Deserializer {
 public:
  template <typename... Args>
  constexpr Deserializer(Args&&... args)
      : reader_{std::forward<Args>(args)...} {}

  constexpr Deserializer(Deserializer&&) = default;
  constexpr Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, &reader_);
  }

  constexpr const Reader& reader() const { return reader_; }
  constexpr Reader& reader() { return reader_; }
  constexpr Reader&& take() { return std::move(reader_); }

 private:
  Reader reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

// Deserializer that wraps a pointer to Reader.
template <typename Reader>
class Deserializer<Reader*> {
 public:
  constexpr Deserializer() : reader_{nullptr} {}
  constexpr Deserializer(Reader* reader) : reader_{reader} {}
  constexpr Deserializer(const Deserializer&) = default;
  constexpr Deserializer& operator=(const Deserializer&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_);
  }

  constexpr const Reader& reader() const { return *reader_; }
  constexpr Reader& reader() { return *reader_; }

 private:
  Reader* reader_;
};

// Deserializer that wraps a unique pointer to reader.
template <typename Reader>
class Deserializer<std::unique_ptr<Reader>> {
 public:
  constexpr Deserializer() = default;
  constexpr Deserializer(std::unique_ptr<Reader> reader)
      : reader_{std::move(reader)} {}
  constexpr Deserializer(Deserializer&&) = default;
  constexpr Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_.get());
  }

  constexpr const Reader& reader() const { return *reader_; }
  constexpr Reader& reader() { return *reader_; }

 private:
  std::unique_ptr<Reader> reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_STRING_H_
#define LIBNOP_INCLUDE_NOP_BASE_STRING_H_

#include <string>



namespace nop {

//
// std::basic_string<...> encoding format:
//
// +-----+---------+---//----+
// | STR | INT64:N | N BYTES |
// +-----+---------+---//----+
//

template <typename CharType, typename Traits, typename Allocator>
struct Encoding<std::basic_string<CharType, Traits, Allocator>>
    : EncodingIO<std::basic_string<CharType, Traits, Allocator>> {
  using Type = std::basic_string<CharType, Traits, Allocator>;
  enum : std::size_t { CharSize = sizeof(CharType) };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::String;
  }

  static std::size_t Size(const Type& value) {
    const std::size_t length_bytes = value.length() * CharSize;
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(length_bytes) + length_bytes;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::String;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const std::size_t length = value.length();
    const std::size_t length_bytes = length * CharSize;
    auto status = Encoding<SizeType>::Write(length_bytes, writer);
    if (!status)
      return status;

    return writer->Write(&value[0], &value[length]);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType length_bytes = 0;
    auto status = Encoding<SizeType>::Read(&length_bytes, reader);
    if (!status)
      return status;
    else if (length_bytes % CharSize != 0)
      return ErrorStatus::InvalidStringLength;

    const SizeType size = length_bytes / CharSize;

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous string sizes.
    status = reader->Ensure(size);
    if (!status)
      return status;

    value->resize(size);
    return reader->Read(&(*value)[0], &(*value)[size]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_STRING_H_


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

/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_BASE_VALUE_H_
#define LIBNOP_INCLUDE_NOP_BASE_VALUE_H_






namespace nop {

//
// Value wrapper around type T format:
//
// +-------+
// | VALUE |
// +-------+
//
// VALUE must be a valid encoding of type T.
//

template <typename T>
struct Encoding<T, EnableIfIsValueWrapper<T>> : EncodingIO<T> {
  using MemberList = typename ValueWrapperTraits<T>::MemberList;
  using Pointer = typename ValueWrapperTraits<T>::Pointer;
  using Type = typename Pointer::Type;

  static constexpr EncodingByte Prefix(const T& value) {
    return Encoding<Type>::Prefix(Pointer::Resolve(value));
  }

  static constexpr std::size_t Size(const T& value) {
    return Pointer::Size(value);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<Type>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const T& value, Writer* writer) {
    return Pointer::WritePayload(prefix, value, writer, MemberList{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, T* value,
                                            Reader* reader) {
    return Pointer::ReadPayload(prefix, value, reader, MemberList{});
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VALUE_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_VARIANT_H_
#define LIBNOP_INCLUDE_NOP_BASE_VARIANT_H_




namespace nop {

//
// Variant<Ts...> encoding format:
//
// +-----+---------+-----------+
// | VAR | INT32:I | ELEMENT I |
// +-----+---------+-----------+
//
// Elements are expected to be valid encodings for their element type.
//
// EmptyVariant encoding format:
//
// +-----+
// | NIL |
// +-----+
//
// Therefore a Variant in the empty state has this specific encoding:
//
// +-----+----+-----+
// | VAR | -1 | NIL |
// +-----+----+-----+
//

template <>
struct Encoding<EmptyVariant> : EncodingIO<EmptyVariant> {
  static constexpr EncodingByte Prefix(EmptyVariant /*value*/) {
    return EncodingByte::Nil;
  }

  static constexpr std::size_t Size(EmptyVariant value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Nil;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             EmptyVariant /*value*/,
                                             Writer* /*writer*/) {
    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            EmptyVariant* /*value*/,
                                            Reader* /*reader*/) {
    return {};
  }
};

template <typename... Ts>
struct Encoding<Variant<Ts...>> : EncodingIO<Variant<Ts...>> {
  using Type = Variant<Ts...>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Variant;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::int32_t>::Size(value.index()) +
           value.Visit([](const auto& element) {
             using Element = typename std::decay<decltype(element)>::type;
             return Encoding<Element>::Size(element);
           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Variant;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<std::int32_t>::Write(value.index(), writer);
    if (!status)
      return status;

    return value.Visit([writer](const auto& element) {
      using Element = typename std::decay<decltype(element)>::type;
      return Encoding<Element>::Write(element, writer);
    });
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    std::int32_t type = 0;
    auto status = Encoding<std::int32_t>::Read(&type, reader);
    if (!status) {
      return status;
    } else if (type < Type::kEmptyIndex ||
               type >= static_cast<std::int32_t>(sizeof...(Ts))) {
      return ErrorStatus::UnexpectedVariantType;
    }

    value->Become(type);

    return value->Visit([reader](auto&& element) {
      using Element = typename std::decay<decltype(element)>::type;
      return Encoding<Element>::Read(&element, reader);
    });
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VARIANT_H_

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

#ifndef LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_
#define LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_

#include <numeric>
#include <vector>




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

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const T& element) {
                             return sum + Encoding<T>::Size(element);
                           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
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
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    // Clear the vector to make sure elements are inserted at the correct
    // indices. Intentionally avoid calling reserve() to prevent abuse from very
    // large size values. Regardless of the size specified in the encoding the
    // bytes remaining in the reader provide a natural upper limit to the number
    // of allocations.
    value->clear();
    for (SizeType i = 0; i < size; i++) {
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

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const SizeType size = value.size() * sizeof(T);
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(size) +
           size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const SizeType length = value.size();
    const SizeType length_bytes = length * sizeof(T);
    auto status = Encoding<SizeType>::Write(length_bytes, writer);
    if (!status)
      return status;

    return writer->Write(&value[0], &value[length]);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    if (size % sizeof(T) != 0)
      return ErrorStatus::InvalidContainerLength;

    const SizeType length = size / sizeof(T);

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous vector sizes.
    status = reader->Ensure(length);
    if (!status)
      return status;

    value->resize(length);
    return reader->Read(&(*value)[0], &(*value)[length]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_


#endif  // LIBNOP_INCLUDE_NOP_SERIALIZER_H_
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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>




namespace nop {

// An efficient reader type that supports runtime serialization from a byte
// buffer. This reader improves efficiency by only performing bounds checks in
// the Ensure() method. This type is safe for use with the library-provided
// Deserializer types, which predicate serialization on the result of Ensure().
// Use PedanticBufferReader if your use case interacts with the reader directly
// and you need bounds checking in the Read() and Skip() methods.
class BufferReader {
 public:
  BufferReader() = default;
  BufferReader(const BufferReader&) = default;
  template <std::size_t Size>
  BufferReader(const std::uint8_t (&buffer)[Size])
      : buffer_{buffer}, size_{Size} {}
  BufferReader(const std::uint8_t* buffer, std::size_t size)
      : buffer_{buffer}, size_{size} {}
  BufferReader(const void* buffer, std::size_t size)
      : buffer_{static_cast<const std::uint8_t*>(buffer)}, size_{size} {}

  BufferReader& operator=(const BufferReader&) = default;

  Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return {};
  }

  Status<void> Read(std::uint8_t* byte) { return Read(byte, byte + 1); }

  template <typename T, typename Enable = EnableIfArithmetic<T>>
  Status<void> Read(T* begin, T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    std::memcpy(begin, &buffer_[index_], length_bytes);
    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    index_ += padding_bytes;
    return {};
  }

  bool empty() const { return index_ == size_; }

  std::size_t remaining() const { return size_ - index_; }
  std::size_t capacity() const { return size_; }

 private:
  const std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
/*
 * Copyright 2017-2019 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>




namespace nop {

// An efficient writer type that supports runtime serialization into a byte
// buffer. This writer improves efficiency by only performing bounds checks in
// the Prepare() method. This type is safe for use with the library-provided
// Serializer types, which predicate serialization on the result of Prepare().
// Use PedanticBufferWriter if your use case interacts with the writer directly
// and you need bounds checking in the Write() and Skip() methods.
class BufferWriter {
 public:
  BufferWriter() = default;
  BufferWriter(const BufferWriter&) = default;
  template <std::size_t Size>
  BufferWriter(std::uint8_t (&buffer)[Size]) : buffer_{buffer}, size_{Size} {}
  BufferWriter(std::uint8_t* buffer, std::size_t size)
      : buffer_{buffer}, size_{size} {}
  BufferWriter(void* buffer, std::size_t size)
      : buffer_{static_cast<std::uint8_t*>(buffer)}, size_{size} {}

  BufferWriter& operator=(const BufferWriter&) = default;

  Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return {};
  }

  Status<void> Write(std::uint8_t byte) { return Write(&byte, &byte + 1); }

  template <typename T, typename Enable = EnableIfArithmetic<T>>
  Status<void> Write(const T* begin, const T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    std::memcpy(&buffer_[index_], begin, length_bytes);
    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    std::memset(&buffer_[index_], padding_value, padding_bytes);
    index_ += padding_bytes;
    return {};
  }

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
/*
 * Copyright 2019 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_

#include <cstddef>
#include <cstdint>




namespace nop {

// A writer type that supports constexpr serialization into a byte buffer. This
// type is sub-optimal for non-constexpr serialization: use BufferWriter or
// PedanticBufferWriter for runtime serialization into a byte buffer.
class ConstexprBufferWriter {
 public:
  constexpr ConstexprBufferWriter() = default;
  constexpr ConstexprBufferWriter(const ConstexprBufferWriter&) = default;
  template <std::size_t Size>
  constexpr ConstexprBufferWriter(std::uint8_t (&buffer)[Size])
      : buffer_{buffer}, size_{Size} {}
  constexpr ConstexprBufferWriter(std::uint8_t* buffer, std::size_t size)
      : buffer_{buffer}, size_{size} {}

  constexpr ConstexprBufferWriter& operator=(const ConstexprBufferWriter&) =
      default;

  constexpr Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return {};
  }

  constexpr Status<void> Write(std::uint8_t byte) {
    if (index_ < size_) {
      buffer_[index_++] = byte;
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  template <typename T, typename Enable = EnableIfArithmetic<T>>
  constexpr Status<void> Write(const T* begin, const T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    for (std::size_t i = 0; i < length; i++)
      WriteElement(begin[i], i * element_size);

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes,
                              std::uint8_t padding_value = 0x00) {
    auto status = Prepare(padding_bytes);
    if (!status)
      return status;

    while (padding_bytes) {
      buffer_[index_++] = padding_value;
      padding_bytes--;
    }

    return {};
  }

  constexpr std::size_t size() const { return index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  // Write an integer element to the byte buffer with constexpr-compatible
  // conversions from larger integer types.
  constexpr void WriteElement(std::uint8_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
  }
  constexpr void WriteElement(std::int8_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint8_t>(value), offset);
  }
  constexpr void WriteElement(char value, std::size_t offset) {
    WriteElement(static_cast<std::uint8_t>(value), offset);
  }
  constexpr void WriteElement(std::uint16_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
  }
  constexpr void WriteElement(std::int16_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint16_t>(value), offset);
  }
  constexpr void WriteElement(std::uint32_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
    buffer_[index_ + offset + 2] = value >> 16;
    buffer_[index_ + offset + 3] = value >> 24;
  }
  constexpr void WriteElement(std::int32_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint32_t>(value), offset);
  }
  constexpr void WriteElement(std::uint64_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
    buffer_[index_ + offset + 2] = value >> 16;
    buffer_[index_ + offset + 3] = value >> 24;
    buffer_[index_ + offset + 4] = value >> 32;
    buffer_[index_ + offset + 5] = value >> 40;
    buffer_[index_ + offset + 6] = value >> 48;
    buffer_[index_ + offset + 7] = value >> 56;
  }
  constexpr void WriteElement(std::int64_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint64_t>(value), offset);
  }

  // TODO(eieio): At the time of this writing there isn't simple way to get the
  // raw bytes of a floating point type in a constexpr expression.

  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_
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

#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_

#include <cstdint>
#include <istream>



namespace nop {

//
// Reader template type that wraps STL input streams.
//
// Implements the basic reader interface on top of an STL input stream type.
//

template <typename IStream>
class StreamReader {
 public:
  template <typename... Args>
  StreamReader(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamReader(const StreamReader&) = default;
  StreamReader& operator=(const StreamReader&) = default;

  Status<void> Ensure(std::size_t /*size*/) { return {}; }

  Status<void> Read(std::uint8_t* byte) {
    using CharType = typename IStream::char_type;
    stream_.read(reinterpret_cast<CharType*>(byte), sizeof(std::uint8_t));

    return ReturnStatus();
  }

  Status<void> Read(void* begin, void* end) {
    using CharType = typename IStream::char_type;
    CharType* begin_char = static_cast<CharType*>(begin);
    CharType* end_char = static_cast<CharType*>(end);

    const std::size_t length_bytes = std::distance(begin_char, end_char);
    stream_.read(begin_char, length_bytes);

    return ReturnStatus();
  }

  Status<void> Skip(std::size_t padding_bytes) {
    stream_.seekg(padding_bytes, std::ios_base::cur);
    return ReturnStatus();
  }

  const IStream& stream() const { return stream_; }
  IStream& stream() { return stream_; }
  IStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus::StreamError;
    else
      return {};
  }

  IStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
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

#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_

#include <cstdint>
#include <ostream>



namespace nop {

//
// Writer template type that wraps STL output streams.
//
// Implements the basic writer interface on top of an STL output stream type.
//

template <typename OStream>
class StreamWriter {
 public:
  template <typename... Args>
  StreamWriter(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamWriter(const StreamWriter&) = default;
  StreamWriter& operator=(const StreamWriter&) = default;

  Status<void> Prepare(std::size_t /*size*/) { return {}; }

  Status<void> Write(std::uint8_t byte) {
    stream_.put(static_cast<typename OStream::char_type>(byte));
    return ReturnStatus();
  }

  Status<void> Write(const void* begin, const void* end) {
    using CharType = typename OStream::char_type;
    const CharType* begin_char = static_cast<const CharType*>(begin);
    const CharType* end_char = static_cast<const CharType*>(end);

    const std::size_t length_bytes = std::distance(begin_char, end_char);
    stream_.write(begin_char, length_bytes);

    return ReturnStatus();
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    for (std::size_t i = 0; i < padding_bytes; i++) {
      stream_.put(padding_value);
      auto status = ReturnStatus();
      if (!status)
        return status;
    }

    return {};
  }

  const OStream& stream() const { return stream_; }
  OStream& stream() { return stream_; }
  OStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus::StreamError;
    else
      return {};
  }

  OStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
