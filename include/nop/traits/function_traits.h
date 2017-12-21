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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_

#include <functional>
#include <tuple>
#include <type_traits>

#include <nop/base/utility.h>
#include <nop/traits/is_fungible.h>

namespace nop {

//
// Function type traits type that captures and compares function signature
// types.
//
// Function traits are used by the serialization engine to store and manipulate
// function signature types and other traits.
//

template <typename T, typename Enabled = void>
struct FunctionTraits;

template <typename Return_, typename... Args_>
struct FunctionTraits<Return_(Args_...)> {
  using Return = Return_;
  using Args = std::tuple<Args_...>;
  using Signature = Identity<Return_(Args_...)>;
  using Function = std::function<Return_(Args_...)>;

  enum : std::size_t { Arity = sizeof...(Args_) };

  template <std::size_t Index>
  using Arg = typename std::tuple_element<Index, Args>::type;

  // Nested trait that determines whether the given signature, function, method,
  // or functor is compatible with the signature of this function trait.
  // Compatible signatures have fungible return and argument types.
  template <typename T, typename Enable = void>
  struct IsCompatible;

  // Specialization for direct signature types.
  template <typename OtherReturn, typename... OtherArgs>
  struct IsCompatible<OtherReturn(OtherArgs...)>
      : IsFungible<Signature, OtherReturn(OtherArgs...)> {};

  // Specialization for functors.
  template <typename T>
  struct IsCompatible<T, Void<decltype(&T::operator())>>
      : IsCompatible<decltype(&T::operator())> {};

  template <typename OtherReturn, typename... OtherArgs>
  struct IsCompatible<OtherReturn (*)(OtherArgs...)>
      : IsFungible<Signature, OtherReturn(OtherArgs...)> {};

  // Specialization for const member function pointers.
  template <typename Class, typename OtherReturn, typename... OtherArgs>
  struct IsCompatible<OtherReturn (Class::*)(OtherArgs...) const>
      : IsFungible<Signature, OtherReturn(OtherArgs...)> {};

  // Specialization for non-const member function pointers.
  template <typename Class, typename OtherReturn, typename... OtherArgs>
  struct IsCompatible<OtherReturn (Class::*)(OtherArgs...)>
      : IsFungible<Signature, OtherReturn(OtherArgs...)> {};

  // Enable if T is compatible with the signature of this function trait.
  template <typename T, typename ReturnType = void>
  using EnableIfCompatible =
      typename std::enable_if<IsCompatible<T>::value, ReturnType>::type;

 private:
  // Skips the first N arguments in the signature type Original and defines a
  // new member type Type with the same return value and remaining arguments.
  template <std::size_t N, typename Original, typename Enabled = void>
  struct SkipArgs;

  // Terminating case. Defines a member Type with the new signature type.
  template <typename Return, typename... Args>
  struct SkipArgs<0, Return(Args...)> {
    using Type = Identity<Return(Args...)>;
  };

  // Recursively skips the first argument until N = 0.
  template <std::size_t N, typename Return, typename First, typename... Rest>
  struct SkipArgs<N, Return(First, Rest...), std::enable_if_t<(N > 0)>>
      : SkipArgs<N - 1, Return(Rest...)> {};

  template <typename A, typename B>
  using IsConformingType = Or<IsFungible<A, B>, std::is_constructible<A, B>>;

  template <typename A, typename B>
  using ConformType = std::conditional_t<
      std::is_constructible<A, B>::value || !IsFungible<A, B>::value, A, B>;

  template <typename T, typename Enable = void>
  struct ConformSignature;

  template <typename OtherReturn, typename... OtherArgs>
  struct ConformSignature<OtherReturn(OtherArgs...),
                          std::enable_if_t<Arity == sizeof...(OtherArgs)>> {
    using Type = Identity<OtherReturn(ConformType<Args_, OtherArgs>...)>;
  };

 public:
  // Returns the signature after trimming the first N leading arguments.
  template <std::size_t N>
  using TrimLeadingArgs = typename SkipArgs<N, Signature>::Type;

  // Nested trait to determine whether the given signature is conforming to the
  // signature of this trait. A conforming signature has types that are either
  // fungible with the types in this signature or can be used to construct types
  // in this signature.
  template <typename T, typename Enable = void>
  struct IsConforming;

  template <typename OtherReturn, typename... OtherArgs>
  struct IsConforming<OtherReturn(OtherArgs...),
                      std::enable_if_t<Arity == sizeof...(OtherArgs)>>
      : And<IsFungible<Return, OtherReturn>,
            IsConformingType<Args_, OtherArgs>...> {};

  // Enable if T is conforming to the signature of this function trait.
  template <typename T, typename ReturnType = void>
  using EnableIfConforming =
      typename std::enable_if<IsConforming<T>::value, ReturnType>::type;

  template <typename OtherSignature>
  using ConformingSignature = typename ConformSignature<OtherSignature>::Type;
};

// Specialization for function pointer types.
template <typename Return, typename... Args>
struct FunctionTraits<Return (*)(Args...)> : FunctionTraits<Return(Args...)> {};

// Specialization for function reference types.
template <typename Return, typename... Args>
struct FunctionTraits<Return (&)(Args...)> : FunctionTraits<Return(Args...)> {};

// Specilization for method pointer types.
template <typename Class, typename Return, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...)>
    : FunctionTraits<Return(Args...)> {};

// Specialization for const method pointer types.
template <typename Class, typename Return, typename... Args>
struct FunctionTraits<Return (Class::*)(Args...) const>
    : FunctionTraits<Return(Args...)> {};

// Specialization for functor types.
template <typename Op>
struct FunctionTraits<Op, Void<decltype(&Op::operator())>>
    : FunctionTraits<decltype(&Op::operator())> {};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
