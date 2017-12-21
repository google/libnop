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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_

#include <cstdint>
#include <memory>

#include <nop/types/optional.h>

namespace nop {

//
// Types for managing thread-local data.
//

// Type tag that takes both a type T and a numeric Index to define thread-local
// slot.
template <typename T, std::size_t Index>
struct ThreadLocalSlot;

// Type tag that takes a type T to define a thread-local slot.
template <typename T>
using ThreadLocalTypeSlot = T;

// Type tag that takes a numeric Index to define a thread-local slot.
template <std::size_t Index>
struct ThreadLocalIndexSlot;

// Defines a thread-local data type with the given type T and type Slot. Each
// unique combination of (T, Slot) defines a different thread-local data type
// that is independent from any other thread-local data with a different (T,
// Slot) combination.
template <typename T, typename Slot = ThreadLocalSlot<void, 0>>
class ThreadLocal {
 public:
  using ValueType = T;

  template <typename... Args>
  ThreadLocal(Args&&... args) : value_{Setup(std::forward<Args>(args)...)} {}

  template <typename... Args>
  void Initialize(Args&&... args) {
    Setup(std::forward<Args>(args)...);
  }

  ValueType& Get() { return value_->get(); }

  void Clear() { value_->clear(); }

 private:
  Optional<ValueType>* value_;

  template <typename... Args>
  static Optional<ValueType>* Setup(Args&&... args) {
    Optional<ValueType>* value = GetValue();
    if (value->empty())
      *value = Optional<ValueType>(std::forward<Args>(args)...);
    return value;
  }

  static Optional<ValueType>* GetValue() {
    static thread_local Optional<ValueType> value;
    return &value;
  }

  ThreadLocal(const ThreadLocal&) = delete;
  void operator=(const ThreadLocal&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_THREAD_LOCAL_H_
