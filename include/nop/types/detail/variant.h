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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_
#define LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_

namespace nop {

// Type tag denoting an empty variant.
struct EmptyVariant {};

namespace detail {

// Type for matching tagged overloads.
template <typename T>
struct TypeTag {};

// Determines the type of the I-th element of Types....
template <std::size_t I, typename... Types>
using TypeForIndex = std::tuple_element_t<I, std::tuple<Types...>>;

// Determines the type tag for the I-th element of Types....
template <std::size_t I, typename... Types>
using TypeTagForIndex = TypeTag<TypeForIndex<I, Types...>>;

// Similar to std::is_constructible but evaluates to false for pointer to
// boolean construction: avoiding this conversion helps prevent subtle bugs in
// Variants with bool elements.
template <typename...>
struct IsConstructible;
template <typename T, typename U>
struct IsConstructible<T, U>
    : std::integral_constant<bool,
                             std::is_constructible<T, U>::value &&
                                 !(std::is_same<std::decay_t<T>, bool>::value &&
                                   std::is_pointer<std::decay_t<U>>::value)> {};
template <typename T, typename... Args>
struct IsConstructible<T, Args...> : std::is_constructible<T, Args...> {};

// Enable if T(Args...) is well formed.
template <typename R, typename T, typename... Args>
using EnableIfConstructible =
    typename std::enable_if<IsConstructible<T, Args...>::value, R>::type;
// Enable if T(Args...) is not well formed.
template <typename R, typename T, typename... Args>
using EnableIfNotConstructible =
    typename std::enable_if<!IsConstructible<T, Args...>::value, R>::type;

// Determines whether T is an element of Types...;
template <typename... Types>
struct HasType : std::false_type {};
template <typename T, typename U>
struct HasType<T, U> : std::is_same<std::decay_t<T>, std::decay_t<U>> {};
template <typename T, typename First, typename... Rest>
struct HasType<T, First, Rest...>
    : std::integral_constant<
          bool, HasType<T, First>::value || HasType<T, Rest...>::value> {};

// Defines set operations on a set of Types...
template <typename... Types>
struct Set {
  // Default specialization catches the empty set, which is always a subset.
  template <typename...>
  struct IsSubset : std::true_type {};
  template <typename T>
  struct IsSubset<T> : HasType<T, Types...> {};
  template <typename First, typename... Rest>
  struct IsSubset<First, Rest...>
      : std::integral_constant<
            bool, IsSubset<First>::value && IsSubset<Rest...>::value> {};
};

// Determines the number of elements of Types... that are constructible from
// From.
template <typename... Types>
struct ConstructibleCount;
template <typename From, typename To>
struct ConstructibleCount<From, To>
    : std::integral_constant<std::size_t, IsConstructible<To, From>::value> {};
template <typename From, typename First, typename... Rest>
struct ConstructibleCount<From, First, Rest...>
    : std::integral_constant<std::size_t,
                             IsConstructible<First, From>::value +
                                 ConstructibleCount<From, Rest...>::value> {};

// Enable if T is an element of Types...
template <typename R, typename T, typename... Types>
using EnableIfElement =
    typename std::enable_if<HasType<T, Types...>::value, R>::type;
// Enable if T is not an element of Types...
template <typename R, typename T, typename... Types>
using EnableIfNotElement =
    typename std::enable_if<!HasType<T, Types...>::value, R>::type;

// Enable if T is convertible to an element of Types... T is considered
// convertible IIF a single element of Types... is assignable from T and T is
// not a direct element of Types...
template <typename R, typename T, typename... Types>
using EnableIfConvertible =
    typename std::enable_if<!HasType<T, Types...>::value &&
                                ConstructibleCount<T, Types...>::value == 1,
                            R>::type;

// Enable if T is assignable to an element of Types... T is considered
// assignable IFF a single element of Types... is constructible from T or T is a
// direct element of Types.... Note that T is REQUIRED to be an element of
// Types... when multiple elements are constructible from T to prevent ambiguity
// in conversion.
template <typename R, typename T, typename... Types>
using EnableIfAssignable =
    typename std::enable_if<HasType<T, Types...>::value ||
                                ConstructibleCount<T, Types...>::value == 1,
                            R>::type;

// Selects a type for SFINAE constructor selection.
template <bool CondA, typename SelectA, typename SelectB>
using Select = std::conditional_t<CondA, SelectA, SelectB>;

// Recursive union type.
template <typename... Types>
union Union {};

// Specialization handling a singular type, terminating template recursion.
template <typename Type>
union Union<Type> {
  Union() {}
  ~Union() {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<Type>, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T, typename = EnableIfAssignable<void, T, Type>>
  Union(std::int32_t index, std::int32_t* index_out, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  Union(const Union& other, std::int32_t index) {
    if (index == 0)
      new (&first_) Type(other.first_);
  }
  Union(Union&& other, std::int32_t index) {
    if (index == 0)
      new (&first_) Type(std::move(other.first_));
  }
  Union(const Union&) = delete;
  Union(Union&&) = delete;
  void operator=(const Union&) = delete;
  void operator=(Union&&) = delete;

  Type& get(TypeTag<Type>) { return first_; }
  const Type& get(TypeTag<Type>) const { return first_; }
  EmptyVariant get(TypeTag<EmptyVariant>) const { return {}; }
  constexpr std::int32_t index(TypeTag<Type>) const { return 0; }

  template <typename... Args>
  std::int32_t Construct(TypeTag<Type>, Args&&... args) {
    new (&first_) Type(std::forward<Args>(args)...);
    return 0;
  }
  template <typename... Args>
  EnableIfConstructible<std::int32_t, Type, Args...> Construct(Args&&... args) {
    new (&first_) Type(std::forward<Args>(args)...);
    return 0;
  }

  void Destruct(std::int32_t target_index) {
    if (target_index == index(TypeTag<Type>{})) {
      (&get(TypeTag<Type>{}))->~Type();
    }
  }

  template <typename T>
  bool Assign(TypeTag<Type>, std::int32_t target_index, T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  EnableIfConstructible<bool, Type, T> Assign(std::int32_t target_index,
                                              T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  EnableIfNotConstructible<bool, Type, T> Assign(std::int32_t /*target_index*/,
                                                 T&& /*value*/) {
    return false;
  }

  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) {
    if (target_index == index(TypeTag<Type>{}))
      return std::forward<Op>(op)(get(TypeTag<Type>{}));
    else
      return std::forward<Op>(op)(get(TypeTag<EmptyVariant>{}));
  }
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) const {
    if (target_index == index(TypeTag<Type>{}))
      return std::forward<Op>(op)(get(TypeTag<Type>{}));
    else
      return std::forward<Op>(op)(get(TypeTag<EmptyVariant>{}));
  }

  template <typename... Args>
  bool Become(std::int32_t target_index, Args&&... args) {
    if (target_index == index(TypeTag<Type>{})) {
      Construct(TypeTag<Type>{}, std::forward<Args>(args)...);
      return true;
    } else {
      return false;
    }
  }

 private:
  Type first_;
};

// Specialization that recursively unions types from the paramater pack.
template <typename First, typename... Rest>
union Union<First, Rest...> {
  Union() {}
  ~Union() {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<First>, T&& value)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T, typename U>
  Union(std::int32_t index, std::int32_t* index_out, TypeTag<T>, U&& value)
      : rest_(index + 1, index_out, TypeTag<T>{}, std::forward<U>(value)) {}
  Union(const Union& other, std::int32_t index) {
    if (index == 0)
      new (&first_) First(other.first_);
    else
      new (&rest_) Union<Rest...>(other.rest_, index - 1);
  }
  Union(Union&& other, std::int32_t index) {
    if (index == 0)
      new (&first_) First(std::move(other.first_));
    else
      new (&rest_) Union<Rest...>(std::move(other.rest_), index - 1);
  }
  Union(const Union&) = delete;
  Union(Union&&) = delete;
  void operator=(const Union&) = delete;
  void operator=(Union&&) = delete;

  struct FirstType {};
  struct RestType {};
  template <typename T>
  using SelectConstructor =
      Select<ConstructibleCount<T, First>::value == 1, FirstType, RestType>;

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value)
      : Union(index, index_out, std::forward<T>(value),
              SelectConstructor<T>{}) {}

  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value, FirstType)
      : first_(std::forward<T>(value)) {
    *index_out = index;
  }
  template <typename T>
  Union(std::int32_t index, std::int32_t* index_out, T&& value, RestType)
      : rest_(index + 1, index_out, std::forward<T>(value)) {}

  First& get(TypeTag<First>) { return first_; }
  const First& get(TypeTag<First>) const { return first_; }
  constexpr std::int32_t index(TypeTag<First>) const { return 0; }

  template <typename T>
  T& get(TypeTag<T>) {
    return rest_.get(TypeTag<T>{});
  }
  template <typename T>
  const T& get(TypeTag<T>) const {
    return rest_.get(TypeTag<T>{});
  }
  template <typename T>
  constexpr std::int32_t index(TypeTag<T>) const {
    return 1 + rest_.index(TypeTag<T>{});
  }

  template <typename... Args>
  std::int32_t Construct(TypeTag<First>, Args&&... args) {
    new (&first_) First(std::forward<Args>(args)...);
    return 0;
  }
  template <typename T, typename... Args>
  std::int32_t Construct(TypeTag<T>, Args&&... args) {
    return 1 +
           rest_.Construct(TypeTag<T>{}, std::forward<Args>(args)...);
  }

  template <typename... Args>
  EnableIfConstructible<std::int32_t, First, Args...> Construct(
      Args&&... args) {
    new (&first_) First(std::forward<Args>(args)...);
    return 0;
  }
  template <typename... Args>
  EnableIfNotConstructible<std::int32_t, First, Args...> Construct(
      Args&&... args) {
    return 1 + rest_.Construct(std::forward<Args>(args)...);
  }

  void Destruct(std::int32_t target_index) {
    if (target_index == index(TypeTag<First>{})) {
      (get(TypeTag<First>{})).~First();
    } else {
      rest_.Destruct(target_index - 1);
    }
  }

  template <typename T>
  bool Assign(TypeTag<First>, std::int32_t target_index, T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return false;
    }
  }
  template <typename T, typename U>
  bool Assign(TypeTag<T>, std::int32_t target_index, U&& value) {
    return rest_.Assign(TypeTag<T>{}, target_index - 1, std::forward<U>(value));
  }
  template <typename T>
  EnableIfConstructible<bool, First, T> Assign(std::int32_t target_index,
                                               T&& value) {
    if (target_index == 0) {
      first_ = std::forward<T>(value);
      return true;
    } else {
      return rest_.Assign(target_index - 1, std::forward<T>(value));
    }
  }
  template <typename T>
  EnableIfNotConstructible<bool, First, T> Assign(std::int32_t target_index,
                                                  T&& value) {
    return rest_.Assign(target_index - 1, std::forward<T>(value));
  }

  // Recursively traverses the union and calls Op on the active value when the
  // active type is found. If the union is empty Op is called on EmptyVariant.
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) {
    if (target_index == index(TypeTag<First>{}))
      return std::forward<Op>(op)(get(TypeTag<First>{}));
    else
      return rest_.Visit(target_index - 1, std::forward<Op>(op));
  }
  template <typename Op>
  decltype(auto) Visit(std::int32_t target_index, Op&& op) const {
    if (target_index == index(TypeTag<First>{}))
      return std::forward<Op>(op)(get(TypeTag<First>{}));
    else
      return rest_.Visit(target_index - 1, std::forward<Op>(op));
  }

  template <typename... Args>
  bool Become(std::int32_t target_index, Args&&... args) {
    if (target_index == index(TypeTag<First>{})) {
      Construct(TypeTag<First>{}, std::forward<Args>(args)...);
      return true;
    } else {
      return rest_.Become(target_index - 1, std::forward<Args>(args)...);
    }
  }

 private:
  First first_;
  Union<Rest...> rest_;
};

}  // namespace detail
}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_DETAIL_VARIANT_H_
