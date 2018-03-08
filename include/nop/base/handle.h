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

#ifndef LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_

#include <nop/base/encoding.h>
#include <nop/types/handle.h>

namespace nop {

//
// Handle<Policy> encoding format:
//
// +-----+------+-----------+
// | HND | TYPE | INT64:REF |
// +-----+------+-----------+
//

template <typename Policy>
struct Encoding<Handle<Policy>> : EncodingIO<Handle<Policy>> {
  using Type = Handle<Policy>;
  using HandleType = decltype(Policy::HandleType());

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Handle;
  }

  static constexpr std::size_t Size(const Type& value) {
    // Overestimate the size as though the handle reference is I64 because the
    // handle reference value, and therefore size, is not known ahead of
    // serialization.
    return BaseEncodingSize(Prefix(value)) +
           Encoding<HandleType>::Size(Policy::HandleType()) +
           BaseEncodingSize(EncodingByte::I64);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Handle;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<HandleType>::Write(Policy::HandleType(), writer);
    if (!status)
      return status;

    auto push_status = writer->template PushHandle<Type>(value);
    if (!push_status)
      return push_status.error();

    return Encoding<HandleReference>::Write(push_status.get(), writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    HandleType handle_type;
    auto status = Encoding<HandleType>::Read(&handle_type, reader);
    if (!status)
      return status;
    else if (handle_type != Policy::HandleType())
      return ErrorStatus::UnexpectedHandleType;

    HandleReference handle_reference = kEmptyHandleReference;
    status = Encoding<HandleReference>::Read(&handle_reference, reader);
    if (!status)
      return status;

    auto get_status = reader->template GetHandle<Type>(handle_reference);
    if (!get_status)
      return get_status.error();

    *value = get_status.take();
    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_HANDLE_H_
