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
  static_assert(!std::is_same<ErrorEnum, std::decay_t<T>>::value,
                "ErrorEnum and T must not be the same type.");

 public:
  Result() : error_{ErrorEnum::None}, state_{State::Error} {}
  Result(const T& value) : value_{value}, state_{State::Value} {}
  Result(T&& value) : value_{std::move(value)}, state_{State::Value} {}
  Result(ErrorEnum error) : error_{error}, state_{State::Error} {}
  Result(const Result& other) { *this = other; }
  Result(Result&& other) { *this = std::move(other); }

  ~Result() { Destruct(); }

  Result& operator=(const Result& other) {
    if (this != &other) {
      if (other.has_value())
        Assign(other.value_);
      else
        Assign(other.error_);
    }
    return *this;
  }
  Result& operator=(Result&& other) {
    if (this != &other) {
      if (other.has_value())
        Assign(std::move(other.value_));
      else
        Assign(other.error_);

      other.Destruct();
    }
    return *this;
  }

  Result& operator=(const T& value) {
    Assign(value);
    return *this;
  }
  Result& operator=(T&& value) {
    Assign(std::move(value));
    return *this;
  }

  bool has_value() const { return state_ == State::Value; }
  bool has_error() const { return state_ == State::Error; }

  explicit operator bool() const { return has_value(); }

  ErrorEnum error() const {
    if (has_error())
      return error_;
    else
      return ErrorEnum::None;
  }

  const T& get() const { return *value(); }
  T& get() { return *value(); }
  T&& take() { return std::move(*value()); }

  void clear() { Destruct(); }

 private:
  void Assign(const T& value) {
    if (has_value()) {
      value_ = value;
    } else {
      new (&value_) T(value);
      state_ = State::Value;
    }
  }

  void Assign(T&& value) {
    if (has_value()) {
      value_ = std::move(value);
    } else {
      new (&value_) T(std::move(value));
      state_ = State::Value;
    }
  }

  void Assign(ErrorEnum error) {
    Destruct();
    error_ = error;
    state_ = State::Error;
  }

  void Destruct() {
    if (has_value()) {
      value_.~T();
      state_ = State::Error;
    }
    error_ = ErrorEnum::None;
  }

  const T* value() const { return has_value() ? &value_ : nullptr; }
  T* value() { return has_value() ? &value_ : nullptr; }

  enum class State {
    Error,
    Value,
  };

  union {
    ErrorEnum error_;
    T value_;
  };
  State state_;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_
