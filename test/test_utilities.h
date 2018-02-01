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

#ifndef LIBNOP_TEST_TEST_UTILITIES_H_
#define LIBNOP_TEST_TEST_UTILITIES_H_

#include <cstdint>
#include <utility>
#include <vector>

#include <nop/base/utility.h>

namespace nop {

template <typename I, typename Enabled = EnableIfIntegral<I>>
inline std::vector<std::uint8_t> Integer(I integer) {
  std::vector<uint8_t> vector(sizeof(I));
  const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&integer);
  for (std::size_t i = 0; i < sizeof(I); i++)
    vector[i] = bytes[i];
  return vector;
}

template <typename F,
          typename Enabled = std::enable_if_t<std::is_floating_point<F>::value>>
inline std::vector<std::uint8_t> Float(F value) {
  std::vector<uint8_t> vector(sizeof(F));
  const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&value);
  for (std::size_t i = 0; i < sizeof(F); i++)
    vector[i] = bytes[i];
  return vector;
}

inline std::vector<std::uint8_t> Item(const std::vector<std::uint8_t>& vector) {
  return vector;
}

// Only define Item for one-byte integral type. Larger integral literals must
// use Integer() explicitly.
inline auto Item(std::uint8_t value) { return Integer(value); }

inline std::vector<std::uint8_t> Item(EncodingByte prefix) {
  return {static_cast<std::uint8_t>(prefix)};
}

inline auto Item(const std::string& string) {
  return std::vector<std::uint8_t>(string.begin(), string.end());
}

inline auto Item(const std::wstring& string) {
  return std::vector<std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(&*string.begin()),
      reinterpret_cast<const std::uint8_t*>(&*string.end()));
}

inline auto Item(const std::u16string& string) {
  return std::vector<std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(&*string.begin()),
      reinterpret_cast<const std::uint8_t*>(&*string.end()));
}

inline auto Item(const std::u32string& string) {
  return std::vector<std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(&*string.begin()),
      reinterpret_cast<const std::uint8_t*>(&*string.end()));
}

inline void Append(std::vector<std::uint8_t>* /*vector*/) {}

template <typename First, typename... Rest>
inline void Append(std::vector<std::uint8_t>* vector, First&& first,
                   Rest&&... rest) {
  auto value = Item(std::forward<First>(first));
  vector->insert(vector->end(), value.begin(), value.end());
  Append(vector, std::forward<Rest>(rest)...);
}

template <typename... Args>
inline std::vector<std::uint8_t> Compose(Args&&... args) {
  std::vector<uint8_t> vector;
  Append(&vector, std::forward<Args>(args)...);
  return vector;
}

}  // namespace nop

#endif  // LIBNOP_TEST_TEST_UTILITIES_H_
