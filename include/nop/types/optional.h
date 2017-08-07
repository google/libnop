#ifndef LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_

#include <type_traits>
#include <utility>

namespace nop {

// Optional single-value container type. This type may either be empty or
// contain a valid instance of type T.
//
// Optional<T> has several useful properties:
//   1. An empty Optional<T> does not default construct its internal T value,
//      instead leaving the memory uninitialized via union semantics. This
//      avoids unnecessary initialization costs and dynamic allocation in the
//      case that type T performs internal dynamic allocation when constructed.
//   2. Optional<T> adds only one byte to the static size of type T. Alignment
//      requirements may still result in additional padding.
//   3. When serialized, an empty Optional<T> may be more compact than the most
//      minimal encoding of type T. For example, when T is std::array<char, 256>
//      the minimal encoding of T is 258 bytes, making Optional<T> a good option
//      to save space during serialization when the value is not always
//      necessary. Conversely, the minimal encoding of an empty std::string is
//      two bytes, making Optional<T> less helpful for strings.
//
template <typename T>
class Optional {
 public:
  Optional() {}
  explicit Optional(const T& value) : value_{value}, empty_{false} {}
  explicit Optional(T&& value) : value_{std::move(value)}, empty_{false} {}
  Optional(const Optional& other) { *this = other; }
  Optional(Optional&& other) { *this = std::move(other); }

  template <typename U, typename Enabled = typename std::enable_if<
                            !std::is_same<std::decay_t<U>, T>::value &&
                            !std::is_same<U, Optional>::value>::type>
  explicit Optional(U&& value)
      : value_{std::forward<U>(value)}, empty_{false} {}
  template <typename... Args,
            typename Enabled = typename std::enable_if<
                sizeof...(Args) >= 0 &&
                std::is_constructible<T, Args...>::value>::type>
  Optional(Args&&... args)
      : value_{std::forward<Args>(args)...}, empty_{false} {}
  template <typename U,
            typename Enabled = typename std::enable_if<std::is_constructible<
                T, std::initializer_list<U>>::value>::type>
  Optional(std::initializer_list<U>&& initializer)
      : value_{std::forward<std::initializer_list<U>>(initializer)},
        empty_{false} {}

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
    Assign(value);
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

  union {
    T value_;
  };
  bool empty_{true};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
