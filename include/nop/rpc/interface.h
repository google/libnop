#ifndef LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_
#define LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_

#include <cstdint>
#include <tuple>
#include <utility>

#include <nop/base/encoding.h>
#include <nop/base/members.h>
#include <nop/base/tuple.h>
#include <nop/base/utility.h>
#include <nop/traits/function_traits.h>
#include <nop/types/variant.h>
#include <nop/utility/sip_hash.h>

namespace nop {

template <std::uint64_t Hash_, typename Signature>
struct InterfaceMethod {
  using Traits = FunctionTraits<Signature>;
  using Function = typename Traits::Function;

  template <typename T, typename Return = void>
  using EnableIfCompatible =
      typename Traits::template EnableIfCompatible<T, Return>;

  enum : std::uint64_t { Hash = Hash_ };

  static bool Match(std::uint64_t method_hash) { return method_hash == Hash; }

  template <typename Serializer, typename... Args>
  static Status<void> Invoke(Serializer* serializer, Args&&... args) {
    return Helper<Signature>::Invoke(serializer, std::forward<Args>(args)...);
  }

  template <typename Deserializer>
  static Status<typename Traits::Return> GetReturn(Deserializer* deserializer) {
    return Helper<Signature>::GetReturn(deserializer);
  }

  template <typename Serializer, typename Deserializer, typename... Args>
  static Status<typename Traits::Return> InvokeAndReturn(
      Serializer* serializer, Deserializer* deserializer, Args&&... args) {
    auto status = Invoke(serializer, std::forward<Args>(args)...);
    if (!status)
      return status.error_status();

    return GetReturn(deserializer);
  }

  template <typename Op>
  struct FunctionBinding {
    Op op;

    static bool Match(std::uint64_t method_hash) {
      return InterfaceMethod::Match(method_hash);
    }

    template <typename Deserializer, typename Serializer>
    Status<void> Dispatch(Deserializer* deserializer, Serializer* serializer) {
      return Helper<Signature>::Dispatch(deserializer, serializer,
                                         std::forward<Op>(op));
    }
  };

  template <typename Class, typename Op>
  struct MethodBinding {
    Class* instance;
    Op op;

    static bool Match(std::uint64_t method_hash) {
      return InterfaceMethod::Match(method_hash);
    }

    template <typename Deserializer, typename Serializer>
    Status<void> Dispatch(Deserializer* deserializer, Serializer* serializer) {
      return Helper<Signature>::Dispatch(deserializer, serializer, instance,
                                         std::forward<Op>(op));
    }
  };

  template <typename Op, typename Enable = EnableIfCompatible<Op>>
  static FunctionBinding<Op> Bind(Op&& op) {
    return FunctionBinding<Op>{std::forward<Op>(op)};
  }

  template <typename Class, typename Return, typename... Args,
            typename Enable = EnableIfCompatible<Return(Args...)>>
  static MethodBinding<Class, Return (Class::*)(Args...) const> Bind(
      Class* instance, Return (Class::*op)(Args...) const) {
    return MethodBinding<Class, Return (Class::*)(Args...) const>{instance, op};
  }

  template <typename Class, typename Return, typename... Args,
            typename Enable = EnableIfCompatible<Return(Args...)>>
  static MethodBinding<Class, Return (Class::*)(Args...)> Bind(
      Class* instance, Return (Class::*op)(Args...)) {
    return MethodBinding<Class, Return (Class::*)(Args...)>{instance, op};
  }

 private:
  template <typename>
  struct Helper;

  template <typename Return, typename... Args>
  struct Helper<Return(Args...)> {
    using ArgsTuple = std::tuple<std::decay_t<Args>...>;

    template <typename Serializer>
    static Status<void> Invoke(Serializer* serializer, Args... args) {
      auto status = serializer->Write(Hash);
      if (!status)
        return status;

      return serializer->Write(
          std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <typename Deserializer>
    static Status<Return> GetReturn(Deserializer* deserializer) {
      Return return_value;
      auto status = deserializer->Read(&return_value);
      if (!status)
        return status.error_status();
      else
        return {std::move(return_value)};
    }

    template <typename Deserializer, typename Serializer, typename Op>
    static Status<void> Dispatch(Deserializer* deserializer,
                                 Serializer* serializer, Op&& op) {
      ArgsTuple args;
      auto status = deserializer->Read(&args);
      if (!status)
        return status;

      Return return_value{Call(std::forward<Op>(op), &args,
                               std::make_index_sequence<sizeof...(Args)>{})};

      return serializer->Write(return_value);
    }

    template <typename Deserializer, typename Serializer, typename Class,
              typename Op>
    static Status<void> Dispatch(Deserializer* deserializer,
                                 Serializer* serializer, Class* instance,
                                 Op&& op) {
      ArgsTuple args;
      auto status = deserializer->Read(&args);
      if (!status)
        return status.error_status();

      Return return_value{Call(instance, std::forward<Op>(op), &args,
                               std::make_index_sequence<sizeof...(Args)>{})};

      return serializer->Write(return_value);
    }

    template <typename Op, std::size_t... Is>
    static Return Call(Op&& op, ArgsTuple* args, std::index_sequence<Is...>) {
      return static_cast<Return>(std::forward<Op>(op)(
          std::get<Is>(std::forward<ArgsTuple>(*args))...));
    }

    template <typename Class, typename Op, std::size_t... Is>
    static Return Call(Class* instance, Op&& op, ArgsTuple* args,
                       std::index_sequence<Is...>) {
      return static_cast<Return>((instance->*std::forward<Op>(op))(
          std::get<Is>(std::forward<ArgsTuple>(*args))...));
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
using InterfaceType = typename T::NOP__INTERFACE;

template <typename T>
using InterfaceApiType = typename T::NOP__INTERFACE_API;

template <typename T>
struct Interface {
  using BASE = Interface<T>;

  static std::uint64_t GetInterfaceHash() { return InterfaceType<T>::Hash; }
  static std::string GetInterfaceName() { return InterfaceType<T>::GetName(); }

  template <std::size_t Index>
  static constexpr std::uint64_t GetMethodHash() {
    return InterfaceApiType<T>::template Method<Index>::Hash;
  }
};

template <typename... Bindings>
class InterfaceBindings {
 public:
  enum : std::size_t { Count = sizeof...(Bindings) };

  InterfaceBindings(Bindings&&... bindings)
      : bindings{std::forward<Bindings>(bindings)...} {}

  bool Match(std::uint64_t method_hash) {
    return MatchTable(method_hash, Index<sizeof...(Bindings)>{});
  }

  template <typename Deserializer, typename Serializer>
  Status<void> Dispatch(Deserializer* deserializer, Serializer* serializer) {
    std::uint64_t method_hash;
    auto status = deserializer->Read(&method_hash);
    if (!status)
      return status;

    return DispatchTable(deserializer, serializer, method_hash,
                         Index<sizeof...(Bindings)>{});
  }

 private:
  std::tuple<Bindings...> bindings;

  template <std::size_t Index>
  using At = typename std::tuple_element<Index, decltype(bindings)>::type;

  bool MatchTable(std::uint64_t method_hash, Index<0>) { return false; }

  template <std::size_t index>
  bool MatchTable(std::uint64_t method_hash, Index<index>) {
    if (At<index - 1>::Match(method_hash))
      return true;
    else
      return MatchTable(method_hash, Index<index - 1>{});
  }

  template <typename Deserializer, typename Serializer>
  Status<void> DispatchTable(Deserializer* deserializer, Serializer* serializer,
                             std::uint64_t method_hash, Index<0>) {
    return ErrorStatus(EOPNOTSUPP);
  }

  template <typename Deserializer, typename Serializer, std::size_t index>
  Status<void> DispatchTable(Deserializer* deserializer, Serializer* serializer,
                             std::uint64_t method_hash, Index<index>) {
    if (At<index - 1>::Match(method_hash)) {
      return std::get<index - 1>(bindings).Dispatch(deserializer, serializer);
    } else {
      return DispatchTable(deserializer, serializer, method_hash,
                           Index<index - 1>{});
    }
  }
};

template <typename... Bindings>
InterfaceBindings<Bindings...> BindInterface(Bindings&&... bindings) {
  return {std::forward<Bindings>(bindings)...};
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
