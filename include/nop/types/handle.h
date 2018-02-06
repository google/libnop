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

#include <nop/base/utility.h>

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
