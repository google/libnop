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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_ENDIAN_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_ENDIAN_H_

#include <cstdint>
#include <type_traits>
#include <utility>

//
// Little-endian and big-endian to host-endian conversion utilities. These
// utilities are portable and very efficient on modern compilers.
//
// While the template expressions used here are somewhat abstruse, they are
// based on a very simple shift-or endian-conversion technique that modern
// compilers recognize and optimze well. The additional complexity supports
// standards compliance, constexpr function requirements, and larger integral
// types that may be introduced with future architectures, while avoiding
// platform dependent APIs and unnecessary branches.
//
// The following example demonstrates this conversion technique using a 4-byte
// big-endian integral type:
//
// const std::uint8_t bytes[] = {0x33, 0x22, 0x11, 0x00};
// std::uint32_t host_endian =
//               static_cast<std::uint32_t>(bytes[0]) << 24 |
//               static_cast<std::uint32_t>(bytes[1]) << 16 |
//               static_cast<std::uint32_t>(bytes[2]) <<  8 |
//               static_cast<std::uint32_t>(bytes[3]) <<  0;
// assert(host_endian == 0x33221100);
//
// This idiom works regardless of the endianness of the host because shift
// operators are defined to operate in host-endianness. The compiler recognizes
// this idiom and produces an optimal conversion. On architectures with
// byte-swap instructions modern compilers optimize this sequence to zero or one
// instruction, depending on the host endianness.
//

namespace nop {

// Base type for endian conversions to host endianness.
template <typename T, typename Enable = void>
struct HostEndian;

// Specialization for integral types.
template <typename T>
struct HostEndian<T, std::enable_if_t<std::is_integral<T>::value>> {
  // Returns the given big-endian value in host endianness.
  static constexpr T FromBig(T value) {
    const std::size_t N = sizeof(T);
    union {
      T value;
      std::uint8_t data[N];
    } u{value};
    return FromBig(u.data, std::make_index_sequence<N>{});
  }

  // Returns the given host-endian value in big endianness.
  static constexpr T ToBig(T value) { return FromBig(value); }

  // Returns the given little-endian value in host endianness.
  static constexpr T FromLittle(T value) {
    const std::size_t N = sizeof(T);
    union {
      T value;
      std::uint8_t data[N];
    } u{value};
    return FromLittle(u.data, std::make_index_sequence<N>{});
  }

  // Returns the given host-endian value in little endianness.
  static constexpr T ToLittle(T value) { return FromLittle(value); }

 private:
  // Allow other specializations to access private members.
  template <typename, typename>
  friend struct HostEndian;

  // Utilities to shift-or the little-endian or big-endian bytes into a
  // host-endian value. These use an index sequence to build the byte indices
  // and shift values needed for the shift-or endianness conversion. The use of
  // initializer_list is only to facilitate the index parameter pack expansion,
  // in lieu of C++17 fold expressons which are not as widely supported as of
  // this writing.
  template <std::size_t N, std::size_t... Is>
  static constexpr T FromLittle(const std::uint8_t (&value)[N],
                                std::index_sequence<Is...>) {
    T out = 0;
    (void)std::initializer_list<bool>{
        (out |= static_cast<T>(value[Is]) << Is * 8, false)...};
    return out;
  }
  template <std::size_t N, std::size_t... Is>
  static constexpr T FromBig(const std::uint8_t (&value)[N],
                             std::index_sequence<Is...>) {
    T out = 0;
    (void)std::initializer_list<bool>{
        (out |= static_cast<T>(value[Is]) << (N - Is - 1) * 8, false)...};
    return out;
  }
};

// Specialization for floating point types. This type forwards the bytes over to
// the integral version to perform the actual conversion, since there is no
// conversion idiom for floating point types.
//
// Note: The endianness of floating point types is not well defined by
// standards. On most modern architectures the endianness of floating point
// types matches that of integral types, but this is by no means dependable. For
// this reason it is best to avoid using floating point types when portability
// is a concern. Support is included in this library for situations where
// communication is restricted to the same machine or to machines with a known
// set of architectures.
template <typename T>
struct HostEndian<T, std::enable_if_t<std::is_floating_point<T>::value>> {
  static constexpr T FromBig(T value) {
    const std::size_t N = sizeof(T);
    union {
      T value;
      std::uint8_t data[N];
    } input{value};
    union {
      Integral value;
      T native;
    } output{HostEndian<Integral>::FromLittle(input.data,
                                              std::make_index_sequence<N>{})};
    return output.native;
  }

  // Returns the given host-endian value in big endianness.
  static constexpr T ToBig(T value) { return FromBig(value); }

  static constexpr T FromLittle(T value) {
    const std::size_t N = sizeof(T);
    union {
      T value;
      std::uint8_t data[N];
    } input{value};
    union {
      Integral value;
      T native;
    } output{HostEndian<Integral>::FromBig(input.data,
                                           std::make_index_sequence<N>{})};
    return output.native;
  }

  // Returns the given host-endian value in little endianness.
  static constexpr T ToLittle(T value) { return FromLittle(value); }

 private:
  // Type matching utility to map from floating point to integral type of the
  // same size. The type long double is not included because there is no well
  // defined integral of the same size.
  static std::uint32_t IntegralType(float);
  static std::uint64_t IntegralType(double);

  // Integral type that matches the size of floating point type T.
  using Integral = decltype(IntegralType(std::declval<T>()));
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_ENDIAN_H_
