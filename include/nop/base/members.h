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

// Determines whether type T has a nested type named NOP__MEMBERS of
// template type MemberList.
template <typename, typename = void>
struct HasMemberList : std::false_type {};
template <typename T>
struct HasMemberList<T, Void<typename T::NOP__MEMBERS>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<MemberList, typename T::NOP__MEMBERS>::value> {
};

// Enable if type T has a nested type called NOP__MEMBERS of type
// MemberList<...>.
template <typename T, typename ReturnType = void>
using EnableIfHasMemberList =
    typename std::enable_if<HasMemberList<T>::value, ReturnType>::type;

template <typename T, typename ReturnType = void>
using EnableIfNotHasMemberList =
    typename std::enable_if<!HasMemberList<T>::value, ReturnType>::type;

// Utility macro to define a MemberPointer type from a type and member name.
#define NOP_MEMBER_POINTER(type, member) \
  ::nop::MemberPointer<decltype(&type::member), &type::member>

// Defines a list of MemberPointer types given a type and list of member names.
#define NOP_MEMBER_LIST(type, ... /*members*/) \
  NOP_FOR_EACH_BINARY_LIST(NOP_MEMBER_POINTER, type, __VA_ARGS__)

#define NOP_MEMBERS(type, ... /*members*/) \
  template <typename, typename>            \
  friend struct ::nop::Encoding;           \
  template <typename, typename>            \
  friend struct ::nop::HasMemberList;      \
  using NOP__MEMBERS = ::nop::MemberList<NOP_MEMBER_LIST(type, __VA_ARGS__)>

//
// struct/class T encoding format:
//
// +-----+---------+-----//----+
// | STC | INT64:N | N MEMBERS |
// +-----+---------+-----//----+
//
// Members are expected to be valid encodings of the member type.
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
      return ErrorStatus(EIO);
    else
      return ReadMembers(value, reader, Index<Count>{});
  }

 private:
  enum : std::size_t { Count = T::NOP__MEMBERS::Count };

  template <std::size_t Index>
  using PointerAt = typename T::NOP__MEMBERS::template At<Index>;

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
