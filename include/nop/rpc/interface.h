#ifndef LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_
#define LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_

#include <cstdint>
#include <tuple>
#include <utility>

#include <nop/base/members.h>
#include <nop/base/tuple.h>
#include <nop/base/utility.h>
#include <nop/traits/function_traits.h>
#include <nop/types/variant.h>
#include <nop/utility/sip_hash.h>

namespace nop {

template <std::uint64_t Hash_, typename Signature_>
struct InterfaceMethod {
  enum : std::uint64_t { Hash = Hash_ };

  using Traits = FunctionTraits<Signature_>;
  using Signature = typename Traits::Signature;
  using Return = typename Traits::Return;
  using Args = typename Traits::Args;

  template <typename Serializer, typename... Params>
  static Status<void> Invoke(Serializer* serializer, Params&&... params) {
    return Helper<Signature>::Invoke(serializer,
                                     std::forward<Params>(params)...);
  }

  template <typename Deserializer, typename Op>
  static Status<Return> Dispatch(Deserializer* deserializer, Op&& op) {
    return Helper<Signature>::Dispatch(deserializer, std::forward<Op>(op));
  }

  template <typename Op>
  struct Binding {
    using Return = typename Traits::Return;
    Op op;

    static bool Match(std::uint64_t method_hash) { return method_hash == Hash; }

    template <typename Deserializer>
    Status<Return> Dispatch(Deserializer* deserializer) {
      return InterfaceMethod::Dispatch(deserializer, op);
    }
  };

  template <typename Op>
  static Binding<Op> Bind(Op&& op) {
    return Binding<Op>{std::forward<Op>(op)};
  }

 private:
  template <typename>
  struct Helper;

  template <typename Return, typename... Params>
  struct Helper<Return(Params...)> {
    using ParamsTuple = std::tuple<std::decay_t<Params>...>;

    template <typename Serializer>
    static Status<void> Invoke(Serializer* serializer, Params&&... params) {
      return serializer->Write(
          std::forward_as_tuple(std::forward<Params>(params)...));
    }

    template <typename Deserializer, typename Op>
    static Status<Return> Dispatch(Deserializer* deserializer, Op&& op) {
      ParamsTuple params;
      auto status = deserializer->Read(&params);
      if (!status)
        return status.error_status();

      return {Call(std::forward<Op>(op), &params,
                   std::make_index_sequence<sizeof...(Params)>{})};
    }

    template <typename Op, std::size_t... Is>
    static Return Call(Op&& op, ParamsTuple* params,
                       std::index_sequence<Is...>) {
      return static_cast<Return>(std::forward<Op>(op)(
          std::get<Is>(std::forward<ParamsTuple>(*params))...));
    }
  };
};

template <typename... InterfaceMethods>
struct InterfaceAPI {
  using Methods = std::tuple<InterfaceMethods...>;
  enum : std::size_t { Length = sizeof...(InterfaceMethods) };

  template <std::size_t Index>
  using Method = typename std::tuple_element<Index, Methods>::type;
};

template <typename T>
using GetInterface = typename T::NOP__INTERFACE;

template <typename T>
using GetInterfaceAPI = typename T::NOP__INTERFACE_API;

#define NOP_EXPAND_PACK(...) [](...) {}(__VA_ARGS__)

template <typename T>
struct Interface {
  using BASE = Interface<T>;

  static std::uint64_t GetInterfaceHash() { return GetInterface<T>::Hash; }
  static std::string GetInterfaceName() { return GetInterface<T>::GetName(); }

  template <std::size_t Index>
  static constexpr std::uint64_t GetMethodHash() {
    return GetInterfaceAPI<T>::template Method<Index>::Hash;
  }
};

template <typename... Bindings>
class Binding {
 public:
  enum : std::size_t { Count = sizeof...(Bindings) };
  using ReturnTypes = Variant<typename Bindings::Return...>;

  Binding(Bindings&&... bindings)
      : bindings{std::forward<Bindings>(bindings)...} {}

  template <typename Deserializer>
  Status<ReturnTypes> Dispatch(Deserializer* deserializer,
                               std::uint64_t method_hash) {
    return DispatchTable(deserializer, method_hash,
                         Index<sizeof...(Bindings)>{});
  }

 private:
  std::tuple<Bindings...> bindings;

  template <std::size_t Index>
  using At = typename std::tuple_element<Index, decltype(bindings)>::type;

  template <typename Deserializer>
  Status<ReturnTypes> DispatchTable(Deserializer* deserializer,
                                    std::uint64_t method_hash, Index<0>) {
    return ErrorStatus(EOPNOTSUPP);
  }

  template <typename Deserializer, std::size_t index>
  Status<ReturnTypes> DispatchTable(Deserializer* deserializer,
                                    std::uint64_t method_hash, Index<index>) {
    if (At<index - 1>::Match(method_hash)) {
      auto status = std::get<index - 1>(bindings).Dispatch(deserializer);
      if (!status)
        return status.error_status();
      else
        return {ReturnTypes(status.take())};
    } else {
      return DispatchTable(deserializer, method_hash, Index<index - 1>{});
    }
  }
};

// TODO(eieio): Prevent duplicate bindings.
template <typename... Bindings>
static Binding<Bindings...> BindInterface(Bindings&&... bindings) {
  return Binding<Bindings...>{std::forward<Bindings>(bindings)...};
}

enum : std::uint64_t {
  kNopInterfaceKey0 = 0xdeadcafebaadf00d,
  kNopInterfaceKey1 = 0x0123456789abcdef,
};

#define NOP_INTERFACE(string_name)                                          \
  struct NOP__INTERFACE {                                                   \
    enum : std::uint64_t {                                                  \
      Hash = ::nop::SipHash::Compute(string_name, ::nop::kNopInterfaceKey0, \
                                     ::nop::kNopInterfaceKey1)              \
    };                                                                      \
    static std::string GetName() { return string_name; }                    \
  }

#define NOP_METHOD(name, ... /* signature */)                          \
  using name = ::nop::InterfaceMethod<::nop::SipHash::Compute(         \
                                          #name, NOP__INTERFACE::Hash, \
                                          ::nop::kNopInterfaceKey1),   \
                                      __VA_ARGS__>

#define NOP_API(... /* methods */) \
  using NOP__INTERFACE_API = ::nop::InterfaceAPI<__VA_ARGS__>

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_
