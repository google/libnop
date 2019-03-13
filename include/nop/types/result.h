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
