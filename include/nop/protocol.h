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
  static Status<void> Write(Serializer* serializer, const T& value) {
    return serializer->Write(value);
  }

  template <typename Deserializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static Status<void> Read(Deserializer* deserializer, T* value) {
    return deserializer->Read(value);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_PROTOCOL_H_
