#ifndef LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_

#include <utility>

namespace nop {

template <typename T>
class Optional {
 public:
  Optional() {}
  explicit Optional(const T& value) : empty_{false}, value_{value} {}
  explicit Optional(T&& value) : empty_{false}, value_{std::move(value)} {}
  Optional(const Optional& other) { *this = other; }
  Optional(Optional&& other) { *this = std::move(other); }

  ~Optional() { Destruct(); }

  Optional& operator=(const Optional& other) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(other.value_);
      } else {
        Destruct();
      }
    }
    return *this;
  }

  Optional& operator=(Optional&& other) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(std::move(other.value_));
        other.Destruct();
      } else {
        Destruct();
      }
    }
    return *this;
  }

  Optional& operator=(const T& value) {
    Assign(std::move(value));
    return *this;
  }

  Optional& operator=(T&& value) {
    Assign(std::move(value));
    return *this;
  }

  bool empty() const { return empty_; }
  explicit operator bool() const { return !empty(); }

  // These accessors may only be called when non-empty.
  const T& get() const { return *value(); }
  T& get() { return *value(); }
  T&& take() { return std::move(*value()); }

  void clear() { Destruct(); }

 private:
  void Assign(const T& value) {
    if (empty()) {
      new (&value_) T(value);
      empty_ = false;
    } else {
      value_ = value;
    }
  }

  void Assign(T&& value) {
    if (empty()) {
      new (&value_) T(std::move(value));
      empty_ = false;
    } else {
      value_ = value;
    }
  }

  void Destruct() {
    if (!empty()) {
      value_.~T();
      empty_ = true;
    }
  }

  const T* value() const { return empty() ? nullptr : &value_; }
  T* value() { return empty() ? nullptr : &value_; }

  bool empty_{true};
  union {
    T value_;
  };
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
