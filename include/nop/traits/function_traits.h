#ifndef LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_

#include <tuple>
#include <type_traits>

#include <nop/base/utility.h>

namespace nop {

template <typename T>
struct FunctionTraits;

template <typename Return_, typename... Args_>
struct FunctionTraits<Return_(Args_...)> {
  using Return = Return_;
  using Args = std::tuple<Args_...>;
  using Signature = Identity<Return_(Args_...)>;

  enum : std::size_t { Arity = sizeof...(Args_) };

  template <std::size_t Index>
  using Arg = typename std::tuple_element<Index, Args>::type;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_FUNCTION_TRAITS_H_
