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
  SimpleMethodReceiver(Serializer* serializer, Deserializer* deserializer)
      : serializer_{serializer}, deserializer_{deserializer} {}

  template <typename MethodSelector>
  Status<void> GetMethodSelector(MethodSelector* method_selector) {
    return deserializer_->Read(method_selector);
  }

  template <typename... Args>
  Status<void> GetArgs(std::tuple<Args...>* args) {
    return deserializer_->Read(args);
  }

  template <typename Return>
  Status<void> SendReturn(const Return& return_value) {
    return serializer_->Write(return_value);
  }

  const Serializer& serializer() const { return *serializer_; }
  Serializer& serializer() { return *serializer_; }
  const Deserializer& deserializer() const { return *deserializer_; }
  Deserializer& deserializer() { return *deserializer_; }

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
