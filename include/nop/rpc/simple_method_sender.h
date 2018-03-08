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

#ifndef LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_SENDER_H_
#define LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_SENDER_H_

#include <tuple>

namespace nop {

// SimpleMethodSender is a minimal implementation of the Sender type required by
// the remote interface support in nop/rpc/interface.h. This class simply
// serializes the method selector and arguments tuple with the given serializer
// and deserializes the return value using the given deserializer, leaving all
// transport-level concerns to the serializer and deserializer type.
template <typename Serializer, typename Deserializer>
class SimpleMethodSender {
 public:
  constexpr SimpleMethodSender(Serializer* serializer,
                               Deserializer* deserializer)
      : serializer_{serializer}, deserializer_{deserializer} {}

  template <typename MethodSelector, typename Return, typename... Args>
  constexpr void SendMethod(MethodSelector method_selector,
                            Status<Return>* return_value,
                            const std::tuple<Args...>& args) {
    auto status = serializer_->Write(method_selector);
    if (!status) {
      *return_value = status.error();
      return;
    }

    status = serializer_->Write(args);
    if (!status) {
      *return_value = status.error();
      return;
    }

    GetReturn(return_value);
  }

  constexpr const Serializer& serializer() const { return *serializer_; }
  constexpr Serializer& serializer() { return *serializer_; }
  constexpr const Deserializer& deserializer() const { return *deserializer_; }
  constexpr Deserializer& deserializer() { return *deserializer_; }

 private:
  template <typename Return>
  constexpr void GetReturn(Status<Return>* return_status) {
    Return return_value;
    auto status = deserializer_->Read(&return_value);
    if (!status)
      *return_status = status.error();
    else
      *return_status = std::move(return_value);
  }

  constexpr void GetReturn(Status<void>* return_status) { *return_status = {}; }

  Serializer* serializer_;
  Deserializer* deserializer_;
};

template <typename Serializer, typename Deserializer>
SimpleMethodSender<Serializer, Deserializer> MakeSimpleMethodSender(
    Serializer* serializer, Deserializer* deserializer) {
  return {serializer, deserializer};
}

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_RPC_SIMPLE_METHOD_SENDER_H_
