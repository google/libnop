#ifndef LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_

#include <cstdint>
#include <memory>

#include <nop/types/variant.h>

namespace nop {

template <typename T, std::size_t Index>
struct ThreadLocalSlot;

template <typename T>
using ThreadLocalTypeSlot = T;

template <std::size_t Index>
struct ThreadLocalIndexSlot;

template <typename T, typename Slot = ThreadLocalSlot<void, 0>>
class ThreadLocal {
 public:
  using ValueType = T;

  template <typename... Args>
  ThreadLocal(Args&&... args) : value_{Setup(std::forward<Args>(args)...)} {}

  template <typename... Args>
  void Initialize(Args&&... args) {
    value_->Become(0, std::forward<Args>(args)...);
  }

  ValueType& Get() { return std::get<ValueType>(*value_); }

  void Clear() { *value_ = EmptyVariant{}; }

 private:
  Variant<ValueType>* value_;

  template <typename... Args>
  static Variant<ValueType>* Setup(Args&&... args) {
    Variant<ValueType>* variant = GetVariant();
    variant->Become(0, std::forward<Args>(args)...);
    return variant;
  }

  static Variant<ValueType>* GetVariant() {
    static thread_local Variant<ValueType> value;
    return &value;
  }

  ThreadLocal(const ThreadLocal&) = delete;
  void operator=(const ThreadLocal&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_
