#ifndef LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_

#include <functional>
#include <tuple>
#include <type_traits>

#include <nop/base/utility.h>
#include <nop/traits/is_fungible.h>

namespace nop {

template <typename T>
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
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
