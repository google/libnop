#ifndef LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_
#define LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_

#include <type_traits>

#include <nop/base/utility.h>

namespace nop {

template <typename T>
using EnumFlagsTraits = decltype(NOP__GetEnumFlagsTraits(std::declval<T*>()));

template <typename, typename = void>
struct IsEnumFlags : std::false_type {};
template <typename T>
struct IsEnumFlags<T, Void<typename EnumFlagsTraits<T>::Type>>
    : std::true_type {};

#define NOP_ENUM_FLAGS(type)                                                  \
  template <typename>                                                         \
  struct NOP__ENUM_FLAGS_TRAITS;                                              \
  template <>                                                                 \
  struct NOP__ENUM_FLAGS_TRAITS<type> {                                       \
    using Type = type;                                                        \
    static_assert(std::is_enum<type>::value,                                  \
                  "Only enum types may be used for enum flags: type=" #type); \
  };                                                                          \
  NOP__ENUM_FLAGS_TRAITS<type> __attribute__((used))                          \
      NOP__GetEnumFlagsTraits(type*)

template <typename T>
using EnableIfEnumFlags = typename std::enable_if<IsEnumFlags<T>::value>::type;

}  // namespace nop

template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator|(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator&(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator^(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) ^ static_cast<U>(b));
}

template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator|=(T& a, T b) {
  a = a | b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator&=(T& a, T b) {
  a = a & b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator^=(T& a, T b) {
  a = a ^ b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator~(T value) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(~static_cast<U>(value));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
bool operator!(T value) {
  using U = typename std::underlying_type<T>::type;
  return !static_cast<U>(value);
}

#endif  // LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_
