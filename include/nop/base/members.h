#ifndef LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_

#include <tuple>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/macros.h>
#include <nop/base/utility.h>

namespace nop {

// Captures the type and value of a pointer to member.
template <typename T, T>
struct MemberPointer;

template <typename T, typename Class, T Class::*Pointer>
struct MemberPointer<T Class::*, Pointer> {
  // Type of the memebr pointed to by this pointer.
  using Type = T;

  // Type of the member pointer this type represents.
  using PointerType = Type Class::*;

  // Resolves a pointer to member with the given instance, yielding a pointer or
  // reference to the member in that instnace.
  static Type* Resolve(Class* instance) { return &(instance->*Pointer); }
  static const Type& Resolve(const Class& instance) {
    return (instance.*Pointer);
  }
};

// Captures a list of MemberPointers.
template <typename... MemberPointers>
struct MemberList {
  using Members = std::tuple<MemberPointers...>;

  enum : std::size_t { Count = std::tuple_size<Members>::value };

  template <std::size_t Index>
  using At = typename std::tuple_element<Index, Members>::type;
};

// Utility to retrieve a traits type that defines a MemberList for type T using
// ADL. The macros NOP_STRUCTURE and NOP_TEMPLATE define the appropriate traits
// type and a defintion of NOP__GetExternalMemberTraits that this utility finds
// using ADL.
template <typename T>
using ExternalMemberTraits =
    decltype(NOP__GetExternalMemberTraits(std::declval<T*>()));

// Work around access check bug in GCC. Keeping original code here to document
// the desired behavior. Bug filed with GCC:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82478
#if 0
// Determines whether type T has a nested type named NOP__MEMBERS of
// template type MemberList.
template <typename, typename = void>
struct HasInternalMemberList : std::false_type {};
template <typename T>
struct HasInternalMemberList<T, Void<typename T::NOP__MEMBERS>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<MemberList, typename T::NOP__MEMBERS>::value> {
  static_assert(std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};
#else
// Determines whether type T has a nested type named NOP__MEMBERS of
// template type MemberList.
template <typename T, typename = void>
struct HasInternalMemberList {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__MEMBERS*) {
    return IsTemplateBaseOf<MemberList, typename U::NOP__MEMBERS>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };

  // Always true if T does not have a NOP__MEMBERS member type. If T does have
  // the member type then only true if T is also default constructible.
  static_assert(!value || std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};
#endif

// Determines whether type T has a properly defined traits type that can be
// discovered by ExternalMemberTraits above.
template <typename, typename = void>
struct HasExternalMemberList : std::false_type {};
template <typename T>
struct HasExternalMemberList<T,
                             Void<typename ExternalMemberTraits<T>::MemberList>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<MemberList, typename ExternalMemberTraits<
                                                 T>::MemberList>::value> {
  static_assert(std::is_default_constructible<T>::value,
                "Serializable types must be default constructible.");
};

// Determines whether a type has either an internal or external MemberList as
// defined by the predicates above.
template <typename T>
struct HasMemberList
    : std::integral_constant<bool, HasInternalMemberList<T>::value ||
                                       HasExternalMemberList<T>::value> {};

// Enable utilities for member list predicates.
template <typename T, typename ReturnType = void>
using EnableIfHasInternalMemberList =
    typename std::enable_if<HasInternalMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfHasExternalMemberList =
    typename std::enable_if<HasExternalMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfHasMemberList =
    typename std::enable_if<HasMemberList<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfNotHasMemberList =
    typename std::enable_if<!HasMemberList<T>::value, ReturnType>::type;

// Traits type that retrieves the internal or external MemberList associated
// with type T.
template <typename T, typename = void>
struct MemberListTraits;
template <typename T>
struct MemberListTraits<T, EnableIfHasInternalMemberList<T>> {
  using MemberList = typename T::NOP__MEMBERS;
};
template <typename T>
struct MemberListTraits<T, EnableIfHasExternalMemberList<T>> {
  using MemberList = typename ExternalMemberTraits<T>::MemberList;
};

// Utility macro to define a MemberPointer type from a type and member name.
#define NOP_MEMBER_POINTER(type, member) \
  ::nop::MemberPointer<decltype(&type::member), &type::member>

// Defines a list of MemberPointer types given a type and list of member names.
#define NOP_MEMBER_LIST(type, ... /*members*/) \
  NOP_FOR_EACH_BINARY_LIST(NOP_MEMBER_POINTER, type, __VA_ARGS__)

// Defines the set of members belonging to a type that should be
// serialized/deserialized. This macro should be called once within the
// struct/class definition, preferrably in the private section for classes with
// private data.
#define NOP_MEMBERS(type, ... /*members*/)    \
  template <typename, typename>               \
  friend struct ::nop::Encoding;              \
  template <typename, typename>               \
  friend struct ::nop::HasInternalMemberList; \
  template <typename, typename>               \
  friend struct ::nop::MemberListTraits;      \
  using NOP__MEMBERS = ::nop::MemberList<NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Defines the set of members belonging to a type that should be
// serialized/deserialized without changing the type itself. This is useful for
// making external library types with public data serializable.
#define NOP_STRUCTURE(type, ... /*members*/)                                  \
  template <typename>                                                         \
  struct NOP__MEMBER_TRAITS;                                                  \
  template <>                                                                 \
  struct NOP__MEMBER_TRAITS<type> {                                           \
    using MemberList = ::nop::MemberList<NOP_MEMBER_LIST(type, __VA_ARGS__)>; \
  };                                                                          \
  NOP__MEMBER_TRAITS<type> __attribute__((used))                              \
      NOP__GetExternalMemberTraits(type*)

// Similar to NOP_STRUCTURE but for template types.
#define NOP_TEMPLATE(type, ... /*members*/)                           \
  template <typename>                                                 \
  struct NOP__MEMBER_TRAITS;                                          \
  template <typename... Ts>                                           \
  struct NOP__MEMBER_TRAITS<type<Ts...>> {                            \
    using MemberList =                                                \
        ::nop::MemberList<NOP_MEMBER_LIST(type<Ts...>, __VA_ARGS__)>; \
  };                                                                  \
  template <typename... Ts>                                           \
  NOP__MEMBER_TRAITS<type<Ts...>> __attribute__((used))               \
      NOP__GetExternalMemberTraits(type<Ts...>*)

//
// struct/class T encoding format:
//
// +-----+---------+-----//----+
// | STC | INT64:N | N MEMBERS |
// +-----+---------+-----//----+
//
// Members are expected to be valid encodings of their member type.
//

template <typename T>
struct Encoding<T, EnableIfHasMemberList<T>> : EncodingIO<T> {
  static constexpr EncodingByte Prefix(const T& value) {
    return EncodingByte::Structure;
  }

  static constexpr std::size_t Size(const T& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(Count) + Size(value, Index<Count>{});
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Structure;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const T& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Count, writer);
    if (!status)
      return status;
    else
      return WriteMembers(value, writer, Index<Count>{});
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, T* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Count)
      return ErrorStatus::InvalidMemberCount;
    else
      return ReadMembers(value, reader, Index<Count>{});
  }

 private:
  enum : std::size_t { Count = MemberListTraits<T>::MemberList::Count };

  template <std::size_t Index>
  using PointerAt =
      typename MemberListTraits<T>::MemberList::template At<Index>;

  static constexpr std::size_t Size(const T& value, Index<0>) { return 0; }

  template <std::size_t index>
  static constexpr std::size_t Size(const T& value, Index<index>) {
    using Pointer = PointerAt<index - 1>;
    using Type = typename Pointer::Type;
    return Size(value, Index<index - 1>{}) +
           Encoding<Type>::Size(Pointer::Resolve(value));
  }

  template <typename Writer>
  static Status<void> WriteMembers(const T& /*value*/, Writer* /*writer*/,
                                   Index<0>) {
    return {};
  }

  template <std::size_t index, typename Writer>
  static Status<void> WriteMembers(const T& value, Writer* writer,
                                   Index<index>) {
    auto status = WriteMembers(value, writer, Index<index - 1>{});
    if (!status)
      return status;

    using Pointer = PointerAt<index - 1>;
    using Type = typename Pointer::Type;
    return Encoding<Type>::Write(Pointer::Resolve(value), writer);
  }

  template <typename Reader>
  static Status<void> ReadMembers(T* /*value*/, Reader* /*reader*/, Index<0>) {
    return {};
  }

  template <std::size_t index, typename Reader>
  static Status<void> ReadMembers(T* value, Reader* reader, Index<index>) {
    auto status = ReadMembers(value, reader, Index<index - 1>{});
    if (!status)
      return status;

    using Pointer = PointerAt<index - 1>;
    using Type = typename Pointer::Type;
    return Encoding<Type>::Read(Pointer::Resolve(value), reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_
