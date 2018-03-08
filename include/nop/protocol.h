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

#ifndef LIBNOP_INCLUDE_NOP_PROTOCOL_H_
#define LIBNOP_INCLUDE_NOP_PROTOCOL_H_

#include <nop/status.h>
#include <nop/traits/is_fungible.h>

namespace nop {

// Implements a simple compile-time type-based protocol check. Overload
// resolution for Write/Read methods succeeds if the argument passed for
// serialization/deserialization is compatible with the protocol type
// ProtocolType.
template <typename ProtocolType>
struct Protocol {
  template <typename Serializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static constexpr Status<void> Write(Serializer* serializer, const T& value) {
    return serializer->Write(value);
  }

  template <typename Deserializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static constexpr Status<void> Read(Deserializer* deserializer, T* value) {
    return deserializer->Read(value);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_PROTOCOL_H_
