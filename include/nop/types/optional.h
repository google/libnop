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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_

#include <initializer_list>
#include <type_traits>
#include <utility>

#include <nop/traits/is_comparable.h>
#include <nop/traits/is_template_base_of.h>

namespace nop {

// Type tag to disambiguate in-place constructors.
struct InPlace {};

// Optional single-value container type. This type may either be empty or
// contain a valid instance of type T.
//
// Optional<T> has several useful properties:
//   1. An empty Optional<T> does not default construct its internal T value,
//      instead leaving the memory uninitialized via union semantics. This
//      avoids unnecessary initialization costs, and dynamic allocation in the
//      case that type T performs internal dynamic allocation when constructed.
//   2. Optional<T> adds only one byte to the static size of type T. Alignment
//      requirements may still result in additional padding.
//   3. When serialized, an empty Optional<T> may be more compact than the most
//      minimal encoding of type T. For example, when T is std::array<char, 128>
//      the minimal encoding of T is 130 bytes, making Optional<T> a good option
//      to save space during serialization when the value is not always
//      necessary. Conversely, the minimal encoding of an empty std::string is
//      two bytes, making Optional<T> less helpful for strings.
//   4. When type T is trivially destructible Optional<T> may be used in
//      constexpr expressions.
//
template <typename T>
class Optional {
  // Ensure that T is not InPlace.
  static_assert(!std::is_same<std::decay_t<T>, InPlace>::value,
                "T cannot be InPlace!");

  // Befriend other Optional types.
  template <typename U>
  friend class Optional;

 public:
  // Default constructs an empty Optional type.
  constexpr Optional() noexcept : state_{} {}

  // Constructs a non-empty Optional from lvalue and rvalue type T.
  constexpr Optional(const T& value) : state_{value} {}
  constexpr Optional(T&& value) : state_{std::move(value)} {}

  // Copy and move constructors.
  constexpr Optional(const Optional& other) : state_{other.state_} {}
  constexpr Optional(Optional&& other) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : state_{std::move(other.state_)} {}

  // Constructs a non-empty Optional from a type U such that T{U()} is valid.
  template <typename U, typename Enabled = std::enable_if_t<
                            std::is_constructible<T, U>::value>>
  constexpr Optional(U&& value) : state_{InPlace{}, std::forward<U>(value)} {}

  // In-place constructor with arbitrary argument forwarding.
  template <typename... Args>
  explicit constexpr Optional(InPlace, Args&&... args)
      : state_{InPlace{}, std::forward<Args>(args)...} {}

  // In-place constructor with initializer list and arbitrary argument
  // forwarding.
  template <typename U, typename... Args,
            typename Enabled = std::enable_if_t<
                std::is_constructible<T, std::initializer_list<U>>::value>>
  constexpr Optional(InPlace, std::initializer_list<U> il, Args&&... args)
      : state_{InPlace{}, il, std::forward<Args>(args)...} {}

  // Constructs a non-empty Optional from a compatible initializer list.
  template <typename U,
            typename Enabled = std::enable_if_t<
                std::is_constructible<T, std::initializer_list<U>>::value>>
  constexpr Optional(std::initializer_list<U> il) : state_{InPlace{}, il} {}

  // Default destructs the Optional. State and Storage nested types below handle
  // destruction for trivial and non-trivial types.
  ~Optional() = default;

  // Copy assignment operator.
  constexpr Optional& operator=(const Optional& other) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(other.state_.storage.value);
      } else {
        Destruct();
      }
    }
    return *this;
  }

  // Move assignment operator.
  constexpr Optional& operator=(Optional&& other) noexcept(
      std::is_nothrow_move_assignable<T>::value&&
          std::is_nothrow_move_constructible<T>::value) {
    if (this != &other) {
      if (!other.empty()) {
        Assign(other.take());
        other.Destruct();
      } else {
        Destruct();
      }
    }
    return *this;
  }

  // Copy assignment from a different Optional type.
  template <typename U>
  constexpr std::enable_if_t<std::is_constructible<T, const U&>::value, Optional&>
  operator=(const Optional<U>& other) {
    if (!other.empty()) {
      Assign(other.get());
    } else {
      Destruct();
    }
    return *this;
  }

  // Move assignment from a different Optional type.
  template <typename U>
  constexpr std::enable_if_t<std::is_constructible<T, U&&>::value, Optional&> operator=(
      Optional<U>&& other) {
    if (!other.empty()) {
      Assign(other.take());
      other.Destruct();
    } else {
      Destruct();
    }
    return *this;
  }

  // Copy/move assignment from type U.
  template <typename U>
  std::enable_if_t<std::is_constructible<T, U>::value, Optional&> operator=(
      U&& value) {
    Assign(std::forward<U>(value));
    return *this;
  }

  // Returns true if the Optional is empty.
  constexpr bool empty() const noexcept { return state_.empty; }

  // Returns true if the Optional is non-empty.
  explicit constexpr operator bool() const noexcept { return !empty(); }

  // Returns the underlying value. These accessors may only be called when
  // non-empty.
  constexpr const T& get() const { return state_.storage.value; }
  constexpr T& get() { return state_.storage.value; }
  constexpr T&& take() { return std::move(state_.storage.value); }

  // Clears the optional to the empty state, destroying the underlying value if
  // necessary.
  void clear() { Destruct(); }

 private:
  // Handles assignment/construction for assignment operators.
  template <typename U>
  constexpr void Assign(U&& value) {
    if (empty()) {
      ::new (&state_.storage.value) T(std::forward<U>(value));
      state_.empty = false;
    } else {
      state_.storage.value = std::forward<U>(value);
    }
  }

  // Destroys the stored value if non-empty.
  constexpr void Destruct() noexcept {
    if (!empty()) {
      state_.storage.value.T::~T();
      state_.empty = true;
    }
  }

  // Type tag used by the nested State type to explicitly select the trivial
  // constructor in the nested Storage type.
  struct TrivialInit {};

  // Base union type used to implement lazy-initialized storage. This type is
  // specialized to handle trivially and non-trivially destructible types.
  template <typename U, typename Enabled = void>
  union Storage;

  // Specialization that handles non-trivially destructible types.
  template <typename U>
  union Storage<U,
                std::enable_if_t<!std::is_trivially_destructible<U>::value>> {
    // Dummy type that is active when the value type is inactive. Since this
    // type is trivially-destructible no special care is necessary when
    // activating the value type.
    unsigned char dummy;

    // Value type that is active when the dummy type is inactive. This type must
    // be properly destroyed before deactivation.
    U value;

    // Trivial constructor that makes the dummy type active.
    constexpr Storage(TrivialInit) noexcept : dummy{} {}

    // Constructs the value type, making it active.
    template <typename... Args>
    constexpr Storage(Args&&... args) noexcept
        : value(std::forward<Args>(args)...) {}

    // Non-trivial destructor. This doesn't do anything useful except enable to
    // compiler to catch attempts to use this type in a non-trivial context.
    ~Storage() {}
  };

  // Specialization that handles trivially-destructible types.
  template <typename U>
  union Storage<U, std::enable_if_t<std::is_trivially_destructible<U>::value>> {
    // Dummy type that is active when the value type is inactive. Since this
    // type is trivially-destructible no special care is necessary when
    // activating the value type.
    unsigned char dummy;

    // Value type that is active when the dummy type is inactive. Since this
    // type is also trivially-destructible no special care is required at
    // deactivation.
    U value;

    // Trivial constructor that makes the dummy type active.
    constexpr Storage(TrivialInit) noexcept : dummy{} {}

    // Constructs the value type, making it active.
    template <typename... Args>
    constexpr Storage(Args&&... args) noexcept
        : value(std::forward<Args>(args)...) {}

    // Trivial destructor.
    ~Storage() = default;
  };

  // Base type to store a lazy-initialized value and track whether it is
  // initialized. This type is specialized to handle trivailly and non-trivially
  // destructible value types.
  template <typename U, typename Enabled = void>
  struct State;

  // Specialization that handles non-trivially destructible types.
  template <typename U>
  struct State<U, std::enable_if_t<!std::is_trivially_destructible<U>::value>> {
    // Default constructor that sets the state to empty and trivially
    // initializes the storage for the value type.
    constexpr State() noexcept : empty{true}, storage{TrivialInit{}} {}

    // Copy constructor. Copy constructs the value type if the other state is
    // non-empty.
    constexpr State(const State& other)
        : empty{other.empty}, storage{TrivialInit{}} {
      if (!other.empty)
        ::new (&storage.value) U(other.storage.value);
    }

    // Move constructor. Move constructs the value type if the other state is
    // non-empty.
    constexpr State(State&& other)
        : empty{other.empty}, storage{TrivialInit{}} {
      if (!other.empty)
        ::new (&storage.value) U(std::move(other.storage.value));
    }

    // Value constructors. Sets the state to non-empty and copies or moves the
    // value to storage.
    explicit constexpr State(const U& value) : empty{false}, storage{value} {}
    explicit constexpr State(U&& value)
        : empty{false}, storage{std::move(value)} {}

    // In-place constructor for more complex initialization.
    template <typename... Args>
    explicit State(InPlace, Args&&... args)
        : empty{false}, storage{std::forward<Args>(args)...} {}

    // In-place initializer list constructor.
    template <typename V, typename... Args,
              typename = std::enable_if_t<
                  std::is_constructible<U, std::initializer_list<V>>::value>>
    explicit State(InPlace, std::initializer_list<V> il, Args&&... args)
        : empty{false}, storage{il, std::forward<Args>(args)...} {}

    // Non-trivial destructor. Destroys the value in storage if non-empty.
    ~State() {
      if (!empty)
        storage.value.U::~U();
    }

    // Tracks whether the storage value is empty (un-initialized) or non-empty
    // (initialized).
    bool empty;

    // Storage for the lazy-initialized value.
    Storage<U> storage;
  };

  // Specialization that handles trivially-destructible types. This
  // specialization conforms with the requirements for constexpr expressions.
  template <typename U>
  struct State<U, std::enable_if_t<std::is_trivially_destructible<U>::value>> {
    // Default constructor that sets the state to empty and trivially
    // initializes the storage for the value type.
    constexpr State() noexcept : empty{true}, storage{TrivialInit{}} {}

    // Copy constructor. Copy constructs the value type if the other state is
    // non-empty.
    constexpr State(const State& other)
        : empty{other.empty}, storage{other.storage.value} {}

    // Move constructor. Move constructs the value type if the other state is
    // non-empty.
    constexpr State(State&& other)
        : empty{other.empty}, storage{std::move(other.storage.value)} {}

    // Value constructors. Sets the state to non-empty and copies or moves the
    // value to storage.
    explicit constexpr State(const U& value) : empty{false}, storage{value} {}
    explicit constexpr State(U&& value)
        : empty{false}, storage{std::move(value)} {}

    // In-place constructor for more complex initialization.
    template <typename... Args>
    explicit constexpr State(InPlace, Args&&... args)
        : empty{false}, storage{std::forward<Args>(args)...} {}

    // In-place initializer list constructor.
    template <typename V, typename... Args,
              typename = std::enable_if_t<
                  std::is_constructible<U, std::initializer_list<V>>::value>>
    explicit constexpr State(InPlace, std::initializer_list<V> il,
                             Args&&... args)
        : empty{false}, storage{il, std::forward<Args>(args)...} {}

    // Trivial destructor.
    ~State() = default;

    // Tracks whether the storage value is empty (un-initialized) or non-empty
    // (initialized).
    bool empty;

    // Storage for the lazy-initialized value.
    Storage<U> storage;
  };

  // Tracks the value and empty/non-empty state.
  State<T> state_;
};

// Relational operators.

template <typename T, typename U,
          typename Enabled = EnableIfComparableEqual<T, U>>
constexpr bool operator==(const Optional<T>& a, const Optional<U>& b) {
  if (a.empty() != b.empty())
    return false;
  else if (a.empty())
    return true;
  else
    return a.get() == b.get();
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableEqual<T, U>>
constexpr bool operator!=(const Optional<T>& a, const Optional<U>& b) {
  return !(a == b);
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator<(const Optional<T>& a, const Optional<U>& b) {
  if (b.empty())
    return false;
  else if (a.empty())
    return true;
  else
    return a.get() < b.get();
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator>(const Optional<T>& a, const Optional<U>& b) {
  return b < a;
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator<=(const Optional<T>& a, const Optional<U>& b) {
  return !(b < a);
}

template <typename T, typename U,
          typename Enabled = EnableIfComparableLess<T, U>>
constexpr bool operator>=(const Optional<T>& a, const Optional<U>& b) {
  return !(a < b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator==(const Optional<T>& a, const U& b) {
  return !a.empty() ? a.get() == b : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator==(const T& a, const Optional<U>& b) {
  return !b.empty() ? a == b.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator!=(const Optional<T>& a, const U& b) {
  return !(a == b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableEqual<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator!=(const T& a, const Optional<U>& b) {
  return !(a == b);
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator<(const Optional<T>& a, const U& b) {
  return !a.empty() ? a.get() < b : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator<(const T& a, const Optional<U>& b) {
  return !b.empty() ? a < b.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator>(const Optional<T>& a, const U& b) {
  return !a.empty() ? b < a.get() : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator>(const T& a, const Optional<U>& b) {
  return !b.empty() ? b.get() < a : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator<=(const Optional<T>& a, const U& b) {
  return !a.empty() ? !(b < a.get()) : true;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator<=(const T& a, const Optional<U>& b) {
  return !b.empty() ? !(b.get() < a) : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, U>::value>>
constexpr bool operator>=(const Optional<T>& a, const U& b) {
  return !a.empty() ? !(a.get() < b) : false;
}

template <
    typename T, typename U,
    typename Enabled = std::enable_if_t<IsComparableLess<T, U>::value &&
                                        !IsTemplateBaseOf<Optional, T>::value>>
constexpr bool operator>=(const T& a, const Optional<U>& b) {
  return !b.empty() ? !(a < b.get()) : true;
}

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_OPTIONAL_H_
