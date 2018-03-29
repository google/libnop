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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_

#include <array>
#include <cstdint>

#include <nop/utility/compiler.h>

// A direct port of the SipHash C reference implementation.
//
// https://131002.net/siphash/siphash24.c
//
// This version supports compile-time constexpr hash computation when provided
// with a byte container that supports constexpr size() and operator[] methods.
//

namespace nop {

// A simple byte container/wrapper with constexpr size() and operator[] methods.
template <typename T>
class BlockReader {
 public:
  static_assert(sizeof(T) == 1, "sizeof(T) != 1");
  using ValueType = T;

  template <std::size_t Size>
  constexpr BlockReader(const ValueType (&value)[Size])
      : data_{value}, size_{Size} {}
  constexpr BlockReader(const ValueType* data, std::size_t size)
      : data_{data}, size_{size} {}

  BlockReader(const BlockReader&) = default;
  BlockReader& operator=(const BlockReader&) = default;

  constexpr std::size_t size() const { return size_; }
  constexpr ValueType operator[](const std::size_t index) const {
    return data_[index];
  }

 private:
  const ValueType* data_;
  std::size_t size_;
};

// Captures a hash value as a compile-time type.
template <std::uint64_t Hash_>
struct HashValue {
  enum : std::uint64_t { Value = Hash_ };
};

struct SipHash {
  template <typename T, std::size_t Size>
  static constexpr std::uint64_t Compute(const T (&buffer)[Size],
                                         std::uint64_t k0, std::uint64_t k1) {
    return Compute(BlockReader<T>(buffer), k0, k1);
  }

  template <typename BufferType>
  static constexpr std::uint64_t Compute(const BufferType buffer,
                                         std::uint64_t k0, std::uint64_t k1) {
    const std::size_t kBlockSize = sizeof(std::uint64_t);
    const std::size_t kLength = buffer.size();
    const std::size_t kLeftOver = kLength % kBlockSize;
    const std::size_t kEndOffset = kLength - kLeftOver;

    std::uint64_t v[4] = {0x736f6d6570736575ULL, 0x646f72616e646f6dULL,
                          0x6c7967656e657261ULL, 0x7465646279746573ULL};

    std::uint64_t b = static_cast<std::uint64_t>(kLength) << 56;

    v[3] ^= k1;
    v[2] ^= k0;
    v[1] ^= k1;
    v[0] ^= k0;

    for (std::size_t offset = 0; offset < kEndOffset; offset += kBlockSize) {
      std::uint64_t m = ReadBlock(buffer, offset);
      v[3] ^= m;
      Round(v);
      Round(v);
      v[0] ^= m;
    }

    switch (kLeftOver) {
      case 7:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 6]) << 48;
        NOP_FALLTHROUGH;
      case 6:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 5]) << 40;
        NOP_FALLTHROUGH;
      case 5:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 4]) << 32;
        NOP_FALLTHROUGH;
      case 4:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 3]) << 24;
        NOP_FALLTHROUGH;
      case 3:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 2]) << 16;
        NOP_FALLTHROUGH;
      case 2:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 1]) << 8;
        NOP_FALLTHROUGH;
      case 1:
        b |= static_cast<std::uint64_t>(buffer[kEndOffset + 0]) << 0;
        NOP_FALLTHROUGH;
      case 0:
        break;
    }

    v[3] ^= b;
    Round(v);
    Round(v);
    v[0] ^= b;

    v[2] ^= 0xff;
    Round(v);
    Round(v);
    Round(v);
    Round(v);
    b = v[0] ^ v[1] ^ v[2] ^ v[3];

    return b;
  }

 private:
  template <typename BufferType>
  static constexpr std::uint64_t ReadBlock(const BufferType buffer,
                                           const std::size_t offset) {
    const std::uint64_t v0 = buffer[offset + 0];
    const std::uint64_t v1 = buffer[offset + 1];
    const std::uint64_t v2 = buffer[offset + 2];
    const std::uint64_t v3 = buffer[offset + 3];
    const std::uint64_t v4 = buffer[offset + 4];
    const std::uint64_t v5 = buffer[offset + 5];
    const std::uint64_t v6 = buffer[offset + 6];
    const std::uint64_t v7 = buffer[offset + 7];

    return ((v7 << 56) | (v6 << 48) | (v5 << 40) | (v4 << 32) | (v3 << 24) |
            (v2 << 16) | (v1 << 8) | (v0 << 0));
  }

  static constexpr std::uint64_t RotateLeft(const std::uint64_t x,
                                            const std::uint64_t b) {
    return (x << b) | (x >> (64 - b));
  }

  static constexpr void Round(std::uint64_t (&v)[4]) {
    v[0] += v[1];
    v[1] = RotateLeft(v[1], 13);
    v[1] ^= v[0];
    v[0] = RotateLeft(v[0], 32);
    v[2] += v[3];
    v[3] = RotateLeft(v[3], 16);
    v[3] ^= v[2];
    v[0] += v[3];
    v[3] = RotateLeft(v[3], 21);
    v[3] ^= v[0];
    v[2] += v[1];
    v[1] = RotateLeft(v[1], 17);
    v[1] ^= v[2];
    v[2] = RotateLeft(v[2], 32);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_SIP_HASH_H_
