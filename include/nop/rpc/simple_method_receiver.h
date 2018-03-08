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

#ifndef LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_RECEIVER_H_
#define LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_RECEIVER_H_

#include <tuple>

namespace nop {

// SimpleMethodReceiver is a minimal implementation of the Receiver type
// required by the remote interface supprt in nop/prc/interface.h. This class
// simply deserializes the method selector and argument tuple from the given
// deserializer and serializes the return value using the given serializer,
// leaving all transport-level concerns to the serializer and deserializer type.
template <typename Serializer, typename Deserializer>
class SimpleMethodReceiver {
 public:
  constexpr SimpleMethodReceiver(Serializer* serializer,
                                 Deserializer* deserializer)
      : serializer_{serializer}, deserializer_{deserializer} {}

  template <typename MethodSelector>
  constexpr Status<void> GetMethodSelector(MethodSelector* method_selector) {
    return deserializer_->Read(method_selector);
  }

  template <typename... Args>
  constexpr Status<void> GetArgs(std::tuple<Args...>* args) {
    return deserializer_->Read(args);
  }

  template <typename Return>
  constexpr Status<void> SendReturn(const Return& return_value) {
    return serializer_->Write(return_value);
  }

  constexpr const Serializer& serializer() const { return *serializer_; }
  constexpr Serializer& serializer() { return *serializer_; }
  constexpr const Deserializer& deserializer() const { return *deserializer_; }
  constexpr Deserializer& deserializer() { return *deserializer_; }

 private:
  Serializer* serializer_;
  Deserializer* deserializer_;
};

template <typename Serializer, typename Deserializer>
SimpleMethodReceiver<Serializer, Deserializer> MakeSimpleMethodReceiver(
    Serializer* serializer, Deserializer* deserializer) {
  return {serializer, deserializer};
}

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_RECEIVER_H_
