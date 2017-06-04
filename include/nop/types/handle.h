#ifndef LIBNOP_INCLUDE_NOP_TYPES_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_TYPES_HANDLE_H_

#include <functional>
#include <type_traits>

#include <nop/base/utility.h>

namespace nop {

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
