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

#ifndef LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_
#define LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include <nop/base/encoding.h>
#include <nop/base/members.h>
#include <nop/base/tuple.h>
#include <nop/base/utility.h>
#include <nop/traits/function_traits.h>
#include <nop/types/variant.h>
#include <nop/utility/sip_hash.h>

namespace nop {

// InterfaceMethod captures the function signature and selector id of a method
// in a remote interface. The signature describes the protocol to use when
// serializing the method for RPC invocation and deserializing the return value.
// The selector is a unique id that distinguishes the method from other methods
// in a particular remote interface. Selectors do not need to be unique across
// interfaces, but if uniqueness holds across interfaces then it is possible to
// group handlers from multiple interfaces into a single dispatch table. It is
// up to the use case to decide whether this makes sense.
//
// This class provides methods for the sending side to invoke the remote method
// provded an appropriately defined Sender type to carry out the
// implementation-specific transport of the RPC. The SimpleMethodSender class
// provides an example of the Sender interface and is useful in simple,
// stream-oriented scenarios.
//
// This class also provides methods for the receiving side to bind handlers for
// the remote method. The return type of these bind methods are primarly useful
// as arguments to the BindInterface function, which collects a group of
// bindings into an instance of the InterfaceBindings class to handle dispatch.
template <typename MethodSelector_, MethodSelector_ Selector_,
          typename Signature>
struct InterfaceMethod {
  // Enforce that the MethodSelector type is integral.
  static_assert(std::is_integral<MethodSelector_>::value,
                "Method selector must be an integral type.");

  // Alias of the method selector type.
  using MethodSelector = MethodSelector_;

  // Alias of the FunctionTraits type for the signature of this interface
  // method.
  using InterfaceTraits = FunctionTraits<Signature>;

  // Enable if the given function type T is compatible (fungible) with the
  // signature of this interface method.
  template <typename T, typename Return = void>
  using EnableIfCompatible =
      typename InterfaceTraits::template EnableIfCompatible<T, Return>;

  // Enable if the given function type T is conforming to the signature of this
  // interface method.
  template <typename T, typename Return = void>
  using EnableIfConforming =
      typename InterfaceTraits::template EnableIfConforming<T, Return>;

  // Conforms the given function type T with the signature of this interface
  // method.
  template <typename T>
  using ConformingSignature =
      typename InterfaceTraits::template ConformingSignature<T>;

  // The selector of this interface method. This is used by Sender and Receiver
  // types to identify a method over the wire and InterfaceBindings to find the
  // requested method in the dispatch table.
  enum : MethodSelector { Selector = Selector_ };

  // Returns true of the given selector matches the selector for this interface
  // method.
  static bool Match(MethodSelector method_selector) {
    return method_selector == Selector;
  }

  // Invokes this interface method using the given sender and arguments.
  template <typename Sender, typename... Args,
            typename Return = typename InterfaceTraits::Return>
  static EnableIfConforming<Return(Args...), Status<Return>> Invoke(
      Sender* sender, Args&&... args) {
    Status<Return> return_value;

    Helper<ConformingSignature<Return(Args...)>>::Invoke(
        sender, &return_value, std::forward<Args>(args)...);

    return return_value;
  }

  template <typename Sender, typename Return, typename... Args>
  static EnableIfConforming<Return(Args...), Status<void>> Invoke(
      Sender* sender, Return* return_value, Args&&... args) {
    Helper<ConformingSignature<Return(Args...)>>::Invoke(
        sender, return_value, std::forward<Args>(args)...);
  }

  // Utility type that deals with the complexity of validating fungible
  // arguments defined by the interface method protocol while accommodating
  // leading passthrough arguments that a handler might receive.
  //
  // Handlers must accept at least as many arguments as defined by the protocol.
  // Any arguments before the protocol-defined arguments are considered
  // passthough and are ignored by the compatibility checks. The
  // protocol-defined arguments can vary from the original protocol types as
  // long as the fungibility constraints are met.
  template <typename HandlerType>
  struct HandlerArgs {
    using HandlerTraits = FunctionTraits<HandlerType>;

    // Ensure that the handler has enough arguments. The static casts here are
    // to address a warning GCC emits when comparing two different anonymous
    // enums, even though both are std::size_t type.
    static_assert(static_cast<std::size_t>(HandlerTraits::Arity) >=
                      static_cast<std::size_t>(InterfaceTraits::Arity),
                  "The handler has fewer arguments than the protocol defines.");

    enum : std::size_t {
      LeadingArgs = HandlerTraits::Arity - InterfaceTraits::Arity
    };

    using TrimmedSignature =
        typename HandlerTraits::template TrimLeadingArgs<LeadingArgs>;
  };

  // Enable if the HandlerType is compatible (fungible) with the signature of
  // this interface method, ignoring any leading passthrough arguments.
  template <typename HandlerType, typename Return = void>
  using EnableIfCompatibleHandler =
      EnableIfCompatible<typename HandlerArgs<HandlerType>::TrimmedSignature,
                         Return>;

  // Nested type that holds a callable handler for receiver-side dispatch of
  // this interface method.
  template <typename Op>
  struct FunctionBinding {
    // Alias of the InterfaceMethod this binding represents.
    using InterfaceMethodType = InterfaceMethod;

    // The callable object or function to invoke to handle the remote method
    // invocation.
    Op op;

    // Returns true if the given selector matches the selector of this interface
    // method.
    static bool Match(MethodSelector method_selector) {
      return InterfaceMethod::Match(method_selector);
    }

    // Deserializes the protocol arguments using the given receiver and executes
    // the handler held by this binding using those arguments, along with the
    // given passthrough arguments. The return value of the handler is then
    // serialized using the given receiver to be returned to the remote caller.
    template <typename Receiver, typename... Passthrough>
    Status<void> Dispatch(Receiver* receiver,
                          Passthrough&&... passthrough) const {
      return Helper<typename FunctionTraits<Op>::Signature>::Dispatch(
          receiver, op, std::forward<Passthrough>(passthrough)...);
    }
  };

  // Nested type that holds a method pointer handler for receiver-side dispatch
  // of this interface method.
  template <typename Class, typename Method>
  struct MethodBinding {
    // Alias of the InterfaceMethod this binding represents.
    using InterfaceMethodType = InterfaceMethod;

    // The method to invoke to handle the remote method invocation.
    Method method;

    // Returns true if the given selector matches the selector of this interface
    // method.
    static bool Match(MethodSelector method_selector) {
      return InterfaceMethod::Match(method_selector);
    }

    // Deserializes the protocol arguments using the given receiver and executes
    // the handler held by this binding using those arguments, along with the
    // given passthrough arguments. The return value of the handler is then
    // serialized using the given receiver to be returned to the remote caller.
    template <typename Receiver, typename... Passthrough>
    Status<void> Dispatch(Receiver* receiver, Class* instance,
                          Passthrough&&... passthrough) const {
      return Helper<typename FunctionTraits<Method>::Signature>::Dispatch(
          receiver, instance, method,
          std::forward<Passthrough>(passthrough)...);
    }
  };

  // Returns an instance of Binding holding the given callable object.
  template <typename Op, typename Enable = EnableIfCompatibleHandler<Op>>
  static constexpr auto Bind(Op&& op) {
    return FunctionBinding<Op>{std::forward<Op>(op)};
  }

  // Returns an instance of Binding holding the given const method pointer.
  template <typename Class, typename Return, typename... Args,
            typename Enable = EnableIfCompatibleHandler<Return(Args...)>>
  static constexpr auto Bind(Return (Class::*op)(Args...) const) {
    return MethodBinding<std::add_const_t<Class>,
                         Return (Class::*)(Args...) const>{op};
  }

  // Returns an instance of Binding holding the given method pointer.
  template <typename Class, typename Return, typename... Args,
            typename Enable = EnableIfCompatibleHandler<Return(Args...)>>
  static constexpr auto Bind(Return (Class::*op)(Args...)) {
    return MethodBinding<Class, Return (Class::*)(Args...)>{op};
  }

 private:
  // Base type for the helper below.
  template <typename>
  struct Helper;

  // Helper class that provides compile-time argument and return type checking
  // when invoking an interface method or dispatching an interface method
  // handler.
  template <typename Return, typename... Args>
  struct Helper<Return(Args...)> {
    using ArgsTuple = std::tuple<std::decay_t<Args>...>;

    // Invokes the remote method using the given sender.
    template <typename Sender>
    static void Invoke(Sender* sender, Status<Return>* return_value,
                       Args... args) {
      sender->template SendMethod(InterfaceMethod::Selector, return_value,
                                  std::forward_as_tuple(args...));
    }

    // Dispatches the given handler op, getting the arguments from the given
    // receiver and passthough arguments and then passing the return value back
    // to the receiver.
    template <typename Receiver, typename Op, typename... Passthrough>
    static Status<void> Dispatch(Receiver* receiver, Op&& op,
                                 Passthrough&&... passthrough) {
      ArgsTuple args;
      auto status = receiver->GetArgs(&args);
      if (!status)
        return status;

      Return return_value{Call(std::forward<Op>(op), &args,
                               std::make_index_sequence<sizeof...(Args)>{},
                               std::forward<Passthrough>(passthrough)...)};

      return receiver->SendReturn(return_value);
    }

    // Dispatches the given handler op, getting the arguments from the given
    // receiver and passthough arguments and then passing the return value back
    // to the receiver.
    template <typename Receiver, typename Class, typename Op,
              typename... Passthrough>
    static Status<void> Dispatch(Receiver* receiver, Class* instance, Op&& op,
                                 Passthrough&&... passthrough) {
      ArgsTuple args;
      auto status = receiver->GetArgs(&args);
      if (!status)
        return status;

      Return return_value{Call(instance, std::forward<Op>(op), &args,
                               std::make_index_sequence<sizeof...(Args)>{},
                               std::forward<Passthrough>(passthrough)...)};

      return receiver->SendReturn(return_value);
    }

    // Helper function to marshall passthough arguments and deserialized
    // arugments to the given handler op.
    template <typename Op, std::size_t... Is, typename... Passthrough>
    static Return Call(Op&& op, ArgsTuple* args, std::index_sequence<Is...>,
                       Passthrough&&... passthrough) {
      // Silence compiler warning in case the handler doesn't have arguments.
      (void)args;

      return static_cast<Return>(std::forward<Op>(op)(
          std::forward<Passthrough>(passthrough)...,
          std::get<Is>(std::forward<ArgsTuple>(*args))...));
    }

    // Helper function to marshall passthough arguments and deserialized
    // arugments to the given handler op.
    template <typename Class, typename Op, std::size_t... Is,
              typename... Passthrough>
    static Return Call(Class* instance, Op&& op, ArgsTuple* args,
                       std::index_sequence<Is...>,
                       Passthrough&&... passthrough) {
      // Silence compiler warning in case the handler doesn't have arguments.
      (void)args;

      return static_cast<Return>((instance->*std::forward<Op>(op))(
          std::forward<Passthrough>(passthrough)...,
          std::get<Is>(std::forward<ArgsTuple>(*args))...));
    }
  };
};

// InterfaceAPI holds a collection of InterfaceMethod types that make up a
// remote interface. This type enforces that each InterfaceMethod is unique,
// which serves to validate the arguments passed to the NOP_API macro.
template <typename... InterfaceMethods>
struct InterfaceAPI {
  template <typename A, typename B>
  using SameSelector = std::integral_constant<bool, A::Selector == B::Selector>;
  static_assert(IsUnique<SameSelector, InterfaceMethods...>::value,
                "Interface method selectors must be unique.");

  // Holds the collection of InterfaceMethod types.
  using Methods = std::tuple<InterfaceMethods...>;

  // The number of methods defined by this remote interface.
  enum : std::size_t { MethodCount = sizeof...(InterfaceMethods) };

  // Looks up an InterfaceMethod type in this collection by numeric index.
  template <std::size_t Index>
  using Method = typename std::tuple_element<Index, Methods>::type;
};

// Evaluates to the interface descriptor type created by the macro
// NOP_INTERFACE(T, ...) give the type T.
template <typename T>
using InterfaceType = typename T::NOP__INTERFACE;

// Evaluates to the InterfaceAPI type created by the macro NOP_API(...) give the
// type T.
template <typename T>
using InterfaceApiType = typename T::NOP__INTERFACE_API;

// Utility type that provides various information about the interface type T.
// Type T must have calls to NOP_INTERFACE(T, ...) and NOP_API(...) within its
// defintion. This type may be subclassed by an interface class to mixin the
// utilites provided here.
template <typename T>
struct Interface {
  // Alias for this type. Useful for subclasses to refer to this type.
  using BASE = Interface<T>;

  // Returns the hash of the interface type T.
  static std::uint64_t GetInterfaceHash() {
    using InterfaceType = typename T::NOP__INTERFACE;
    return InterfaceType::Hash;
  }

  // Returns the name of the interface type T.
  static std::string GetInterfaceName() {
    using InterfaceType = typename T::NOP__INTERFACE;
    return InterfaceType::GetName();
  }

  // Looks up the selector for a method in the interface by numeric index.
  template <std::size_t Index>
  static constexpr auto GetMethodSelector() {
    using InterfaceApiType = typename T::NOP__INTERFACE_API;
    return InterfaceApiType::template Method<Index>::Selector;
  }
};

// Utility type to capture the argument sequence that will be passed through
// during dispatch as the leading arguments to an interface binding handler. For
// handlers that are methods of a class this argument sequence must start with a
// pointer to the class but may include other useful arguments as well.
template <typename... Args>
struct Passthrough {};

// Base type for InterfaceBindings dispatcher class.
template <typename, typename...>
class InterfaceBindings;

// InterfaceBindings provides compile-time dispatch table generation for a set
// or subset of interface method handlers.
template <typename... Args, typename... Bindings>
class InterfaceBindings<Passthrough<Args...>, Bindings...> {
  template <typename A, typename B>
  using SameBinding = std::is_same<typename A::InterfaceMethodType,
                                   typename B::InterfaceMethodType>;
  static_assert(IsUnique<SameBinding, Bindings...>::value,
                "Interface methods cannot be bound to more than one handler.");

  template <typename A, typename B>
  using SameSelectorType =
      std::is_same<typename A::InterfaceMethodType::MethodSelector,
                   typename B::InterfaceMethodType::MethodSelector>;
  static_assert(IsSame<SameSelectorType, Bindings...>::value,
                "Interface methods must have the same selector type.");

 public:
  // The method selector type used by this dispatch table.
  using MethodSelector =
      First<typename Bindings::InterfaceMethodType::MethodSelector...>;

  // The number of handlers in this dispatch table.
  enum : std::size_t { Count = sizeof...(Bindings) };

  // Constructs an instance with the given bindings. Bindings are instances of
  // InterfaceMethod::*Binding returned by the method InterfaceMethod::Bind().
  constexpr InterfaceBindings(Bindings&&... bindings)
      : bindings_{std::forward<Bindings>(bindings)...} {}

  // Returns true if the given selector matches one of the interface methods
  // bound in this dispatch table.
  bool Match(MethodSelector method_selector) {
    return MatchTable(method_selector, Index<sizeof...(Bindings)>{});
  }

  // Attempts to dispatch one of the bound handlers with the given receiver and
  // passthrough args. If the selector does not match one of the bound methods
  // in this dispatch table ErrorStatus::InvalidInterfaceMethod is returned.
  template <typename Receiver>
  Status<void> operator()(Receiver* receiver, Args&&... args) const {
    MethodSelector method_selector;
    auto status = receiver->GetMethodSelector(&method_selector);
    if (!status)
      return status.error();

    return DispatchTable(receiver, method_selector,
                         Index<sizeof...(Bindings)>{},
                         std::forward<Args>(args)...);
  }

 private:
  // The bindings for each interface method in this dispatch table.
  std::tuple<Bindings...> bindings_;

  // Looks up the binding type for a binding in this dispatch table by index.
  template <std::size_t Index>
  using At = typename std::tuple_element<Index, decltype(bindings_)>::type;

  // Terminates recursion when searching for the given method selector. Reaching
  // this function means the selector was not found in this dispatch table.
  template <typename MethodSelector>
  bool MatchTable(MethodSelector /*method_selector*/, Index<0>) const {
    return false;
  }

  // Recurses through the bindings in this dispatch table looking for the given
  // method selector.
  template <typename MethodSelector, std::size_t index>
  bool MatchTable(MethodSelector method_selector, Index<index>) const {
    if (At<index - 1>::Match(method_selector))
      return true;
    else
      return MatchTable(method_selector, Index<index - 1>{});
  }

  // Terminates recursion when searching for the given method selector to
  // dispatch.
  template <typename Receiver, typename MethodSelector>
  Status<void> DispatchTable(Receiver* /*receiver*/,
                             MethodSelector /*method_selector*/, Index<0>,
                             Args&&... /*args*/) const {
    return ErrorStatus::InvalidInterfaceMethod;
  }

  // Recurses through the bindings in this dispatch table looking for the given
  // method selector.
  template <typename Receiver, typename MethodSelector, std::size_t index>
  Status<void> DispatchTable(Receiver* receiver, MethodSelector method_selector,
                             Index<index>, Args&&... args) const {
    if (At<index - 1>::Match(method_selector)) {
      return std::get<index - 1>(bindings_).Dispatch(
          receiver, std::forward<Args>(args)...);
    } else {
      return DispatchTable(receiver, method_selector, Index<index - 1>{},
                           std::forward<Args>(args)...);
    }
  }
};

// Creates a dispatch table with the given bindings. The leading template
// arguments to this function are used to specify the type of passthrough
// arguments that each binding handler must accept.
template <typename... Args, typename... Bindings>
constexpr InterfaceBindings<Passthrough<Args...>, Bindings...> BindInterface(
    Bindings&&... bindings) {
  return {std::forward<Bindings>(bindings)...};
}

// Alias type for a std::function that can hold a dispatch table created by
// BindInterface.
template <typename Receiver, typename... PassthroughArgs>
using InterfaceDispatcher =
    std::function<Status<void>(Receiver*, PassthroughArgs...)>;

// Keys passed to SipHash::Compute when generating method selectors at
// compile-time.
enum : std::uint64_t {
  kNopInterfaceKey0 = 0xdeadcafebaadf00d,
  kNopInterfaceKey1 = 0x0123456789abcdef,
};

template <typename MethodSelector, std::size_t Length>
inline constexpr MethodSelector ComputeMethodSelector(
    const char (&name)[Length], std::uint64_t interface_hash) {
  return static_cast<MethodSelector>(
      SipHash::Compute(name, interface_hash, kNopInterfaceKey1));
}

// Defines the name of an interface and sets up a descriptor type that is used
// by other definitions. This macro must be called within a class or structure
// definition, and may appear in public, protected, or private sections. The
// argument must be a string literal or const char array, which is used to
// compute a compile-time hash for distinguishing between interfaces. It is
// recommended to use a domain-qualified name, for example
// "io.github.eieio.example.InterfaceName", but it is not required as the
// contents are not interpreted directly except to compute the hash.
//
// Example of defining a basic remote interface:
// struct MyInterface {
//   NOP_INTERFACE("io.github.eieio.example.MyInterface");
//   NOP_METHOD(Add, float(float a, floa b));
//   NOP_INTERFACE_API(Add);
// };
//
#define _NOP_INTERFACE(selector_type, string_name)                          \
  template <typename>                                                       \
  friend struct ::nop::Interface;                                           \
  template <typename...>                                                    \
  friend struct ::nop::InterfaceAPI;                                        \
  struct NOP__INTERFACE {                                                   \
    using MethodSelector = selector_type;                                   \
    enum : std::uint64_t {                                                  \
      Hash = ::nop::SipHash::Compute(string_name, ::nop::kNopInterfaceKey0, \
                                     ::nop::kNopInterfaceKey1)              \
    };                                                                      \
    static std::string GetName() { return string_name; }                    \
  }

// Defines an interface with a 64bit method selector type. This should be used
// in most situations, even on 32bit platforms unless resources are constrained.
#define NOP_INTERFACE(string_name) _NOP_INTERFACE(std::uint64_t, string_name)

// Defines an interface with a 32bit method selector type. This is an
// optimiztion intended to small 32bit platforms such as microcontrollers.
// Larger 32bit platforms should use the 64bit variant.
#define NOP_INTERFACE32(string_name) _NOP_INTERFACE(std::uint32_t, string_name)

// Defines a remote method name and signature. This macro must be called within
// a class or structure definition, and may appear in public, protected, or
// private sections. The name argument must be a valid identifier that is unique
// in the enclosing scope. It is used both as the name of a type alias for an
// instantiation of InterfaceMethod and in stringified form to generate a
// compile-time hash that is used as a method selector.
#define NOP_METHOD(name, ... /* signature */)                                  \
  NOP_METHOD_SEL(::nop::ComputeMethodSelector<NOP__INTERFACE::MethodSelector>( \
                     #name, NOP__INTERFACE::Hash),                             \
                 name, __VA_ARGS__)

// Defines a remote method with a manually specified method selector.
#define NOP_METHOD_SEL(selector, name, ... /* signature */)           \
  using name = ::nop::InterfaceMethod<NOP__INTERFACE::MethodSelector, \
                                      selector, __VA_ARGS__>

// Defines the collection of remote methods that comprise the remote interface.
// The arguments to this function must be the symbol names passed to
// NOP_METHOD() within the same class or structure definition.
#define NOP_INTERFACE_API(... /* methods */)                   \
  using NOP__INTERFACE_API = ::nop::InterfaceAPI<__VA_ARGS__>; \
  static_assert(sizeof(NOP__INTERFACE_API) == sizeof(NOP__INTERFACE_API), "")

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_RPC_INTERFACE_H_
