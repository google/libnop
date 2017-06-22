#ifndef LIBNOP_INCLUDE_NOP_STATUS_H_
#define LIBNOP_INCLUDE_NOP_STATUS_H_

#include <algorithm>
#include <memory>
#include <string>

extern "C" int strerror_r(int error, char* buf, size_t buflen);

namespace nop {

// This is a helper class for constructing Status<T> with an error code.
struct ErrorStatus {
 public:
  explicit ErrorStatus(int error) : error_{error} {}
  int error() const { return error_; }

  static std::string ErrorToString(int error_code) {
    char message[1024] = {};
    strerror_r(error_code, message, sizeof(message));
    return message;
  }

 private:
  int error_;
};

// Status<T> is a container class that can be used to return a value of type T
// or error code to the caller.
template <typename T>
class Status {
 public:
  // Default constructor so an empty Status object can be created.
  Status() : error_{-1} {}

  // Value copy/move constructors. These are intentionally not marked as
  // explicit to allow direct value returns from functions without having
  // to explicitly wrap them into Status<T>().
  Status(const T& value) : value_{value} {}        // NOLINT(runtime/explicit)
  Status(T&& value) : value_{std::move(value)} {}  // NOLINT(runtime/explicit)

  // Constructor for storing an error code inside the Status object.
  Status(const ErrorStatus& error_status)  // NOLINT(runtime/explicit)
      : error_{error_status.error()} {}

  // Copy/move constructors. Move constructor leaves |other| object in empty
  // state.
  Status(const Status& other) = default;
  Status(Status&& other)
      : value_{std::move(other.value_)}, error_{other.error_} {
    other.error_ = -1;
  }

  // Assignment operators.
  Status& operator=(const Status& other) = default;
  Status& operator=(Status&& other) {
    error_ = other.error_;
    value_ = std::move(other.value_);
    other.error_ = -1;
    T empty;
    std::swap(other.value_, empty);
    return *this;
  }

  // Change the value/error code of the status object directly.
  void SetValue(T value) {
    error_ = 0;
    value_ = std::move(value);
  }
  void SetError(int error) {
    error_ = error;
    T empty;
    std::swap(value_, empty);
  }

  // If |other| is in error state, copy the error code to this object.
  // Returns true if error was propagated
  template <typename U>
  bool PropagateError(const Status<U>& other) {
    if (!other.ok() && !other.empty()) {
      SetError(other.error());
      return true;
    }
    return false;
  }

  // Returns true if the status object contains valid value for type T.
  // This means, the object is not empty and does not contain an error code.
  bool ok() const { return error_ == 0; }

  // Checks if the object is empty (doesn't contain a valid value nor an error).
  bool empty() const { return error_ < 0; }

  // Explicit bool conversion, equivalent to invoking ok().
  explicit operator bool() const { return ok(); }

  // Accessors for the value stored in Status. Calling when ok() is false leads
  // to undefined behavior.
  const T& get() const { return value_; }
  T&& take() {
    error_ = -1;
    return std::move(value_);
  }

  // Returns the error code stored in the object. These codes are positive
  // non-zero values.
  // Can be called only when an error is actually stored (that is, the object
  // is not empty nor containing a valid value).
  int error() const { return std::max(error_, 0); }

  // Returns the error code as ErrorStatus object. This is a helper method
  // to aid in propagation of error codes between Status<T> of different types
  // as in the following example:
  //    Status<int> foo() {
  //      Status<void> status = bar();
  //      if(!status)
  //        return status.error_status();
  //      return 12;
  //    }
  inline ErrorStatus error_status() const { return ErrorStatus{error()}; }

  // Returns the error message associated with error code stored in the object.
  // The message is the same as the string returned by strerror(status.error()).
  // Can be called only when an error is actually stored (that is, the object
  // is not empty nor containing a valid value).
  std::string GetErrorMessage() const {
    std::string message;
    if (error_ > 0)
      message = ErrorStatus::ErrorToString(error_);
    return message;
  }

 private:
  T value_{};
  int error_{0};
};

// Specialization for status containing no other value but the error code.
template <>
class Status<void> {
 public:
  Status() = default;
  Status(const ErrorStatus& error_status)  // NOLINT(runtime/explicit)
      : error_{error_status.error()} {}
  void SetValue() { error_ = 0; }
  void SetError(int error) { error_ = error; }

  template <typename U>
  bool PropagateError(const Status<U>& other) {
    if (!other.ok() && !other.empty()) {
      SetError(other.error());
      return true;
    }
    return false;
  }

  bool ok() const { return error_ == 0; }
  bool empty() const { return false; }
  explicit operator bool() const { return ok(); }
  int error() const { return std::max(error_, 0); }
  inline ErrorStatus error_status() const { return ErrorStatus{error()}; }
  std::string GetErrorMessage() const {
    std::string message;
    if (error_ > 0)
      message = ErrorStatus::ErrorToString(error_);
    return message;
  }

 private:
  int error_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_STATUS_H_
