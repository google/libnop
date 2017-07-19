#ifndef LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_
#define LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_

#include <type_traits>

namespace nop {

template <typename ErrorEnum, typename T>
class Result {
  static_assert(std::is_enum<ErrorEnum>::value,
                "ErrorEnum must be an enum class.");
  static_assert(!std::is_same<ErrorEnum, std::decay_t<T>>::value,
                "ErrorEnum and T must not be the same type.");
 public:
  Result() : state_{State::Error}, error_{ErrorEnum::None} {}
  Result(const T& value) : state_{State::Value}, value_{value} {}
  Result(T&& value) : state_{State::Value}, value_{std::move(value)} {}
  Result(ErrorEnum error) : state_{State::Error}, error_{error} {}
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

  State state_;
  union {
    ErrorEnum error_;
    T value_;
  };
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPE_RESULT_H_
