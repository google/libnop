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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_
#define LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_

#include <functional>
#include <tuple>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>
#include <nop/types/detail/logical_buffer.h>

namespace nop {

// Captures the type and value of a pointer to member.
template <typename T, T, typename U = void*, U = nullptr,
          typename Enable = void>
struct MemberPointer;

template <typename T, typename Class, T Class::*Pointer>
struct MemberPointer<T Class::*, Pointer> {
  // Type of the memebr pointed to by this pointer.
  using Type = T;

  // Type of the member pointer this type represents.
  using PointerType = Type Class::*;

  // Resolves a pointer to member with the given instance, yielding a pointer or
  // reference to the member in that instnace.
  static constexpr Type* Resolve(Class* instance) {
    return &(instance->*Pointer);
  }
  static constexpr const Type& Resolve(const Class& instance) {
    return (instance.*Pointer);
  }

  // Returns true if the given member pointer is greater or equal to this member
  // pointer. This creates a total order of member pointers within a type.
  template <typename U>
  static constexpr bool GreaterEqual(const Class& instance, U Class::*member) {
    return std::greater_equal<>{}(
        static_cast<const void*>(&(instance.*member)),
        static_cast<const void*>(&(instance.*Pointer)));
  }

  static std::size_t Size(const Class& instance) {
    return Encoding<T>::Size(Resolve(instance));
  }

  template <typename Writer, typename MemberList>
  static Status<void> Write(const Class& instance, Writer* writer,
                            MemberList /*member_list*/) {
    return Encoding<T>::Write(Resolve(instance), writer);
  }

  template <typename Writer, typename MemberList>
  static Status<void> WritePayload(EncodingByte prefix, const Class& instance,
                                   Writer* writer, MemberList /*member_list*/) {
    return Encoding<T>::WritePayload(prefix, Resolve(instance), writer);
  }

  template <typename Reader, typename MemberList>
  static Status<void> Read(Class* instance, Reader* reader,
                           MemberList /*member_list*/) {
    return Encoding<T>::Read(Resolve(instance), reader);
  }

  template <typename Reader, typename MemberList>
  static Status<void> ReadPayload(EncodingByte prefix, Class* instance,
                                  Reader* reader, MemberList /*member_list*/) {
    return Encoding<T>::ReadPayload(prefix, Resolve(instance), reader);
  }
};

// Member pointer type for logical buffers formed by an array and size member
// pair.
template <typename Class, typename First, typename Second,
          First Class::*FirstPointer, Second Class::*SecondPointer>
struct MemberPointer<First Class::*, FirstPointer, Second Class::*,
                     SecondPointer, EnableIfLogicalBufferPair<First, Second>> {
  using Type = LogicalBuffer<First, Second>;
  enum : std::size_t { Length = ArrayTraits<First>::Length };
  enum : bool {
    IsTriviallyDestructible =
        std::is_trivially_destructible<typename Type::ValueType>::value
  };

  static const Type Resolve(const Class& instance) {
    return {const_cast<First&>(instance.*FirstPointer),
            const_cast<Second&>(instance.*SecondPointer)};
  }

  static Type Resolve(Class* instance) {
    return {instance->*FirstPointer, instance->*SecondPointer};
  }

  // Returns true if the given member pointer is greater or equal to this member
  // pointer. This creates a total order of member pointers within a structure
  // or class.
  template <typename U>
  static constexpr bool GreaterEqual(const Class& instance, U Class::*member) {
    return std::greater_equal<>{}(
               static_cast<const void*>(&(instance.*member)),
               static_cast<const void*>(&(instance.*FirstPointer))) &&
           std::greater_equal<>{}(
               static_cast<const void*>(&(instance.*member)),
               static_cast<const void*>(&(instance.*SecondPointer)));
  }

  static std::size_t Size(const Class& instance) {
    const Type pair = Resolve(instance);
    return Encoding<Type>::Size(pair);
  }

  template <typename Writer, typename MemberList>
  static Status<void> Write(const Class& instance, Writer* writer,
                            MemberList /*member_list*/) {
    const bool last_member = MemberList::IsLastMember(instance, FirstPointer);
    const Type pair = Resolve(instance);
    if (last_member && Length == 1 && IsTriviallyDestructible)
      return Encoding<Type>::WriteUnbounded(pair, writer);
    else
      return Encoding<Type>::Write(pair, writer);
  }

  template <typename Writer, typename MemberList>
  static Status<void> WritePayload(EncodingByte prefix, const Class& instance,
                                   Writer* writer, MemberList /*member_list*/) {
    const bool last_member = MemberList::IsLastMember(instance, FirstPointer);
    const Type pair = Resolve(instance);
    if (last_member && Length == 1 && IsTriviallyDestructible)
      return Encoding<Type>::WriteUnboundedPayload(prefix, pair, writer);
    else
      return Encoding<Type>::WritePayload(prefix, pair, writer);
  }

  template <typename Reader, typename MemberList>
  static Status<void> Read(Class* instance, Reader* reader,
                           MemberList /*member_list*/) {
    const bool last_member = MemberList::IsLastMember(*instance, FirstPointer);
    Type pair = Resolve(instance);
    if (last_member && Length == 1 && IsTriviallyDestructible)
      return Encoding<Type>::ReadUnbounded(&pair, reader);
    else
      return Encoding<Type>::Read(&pair, reader);
  }

  template <typename Reader, typename MemberList>
  static Status<void> ReadPayload(EncodingByte prefix, Class* instance,
                                  Reader* reader, MemberList /*member_list*/) {
    const bool last_member = MemberList::IsLastMember(*instance, FirstPointer);
    Type pair = Resolve(instance);
    if (last_member && Length == 1 && IsTriviallyDestructible)
      return Encoding<Type>::ReadUnboundedPayload(prefix, &pair, reader);
    else
      return Encoding<Type>::ReadPayload(prefix, &pair, reader);
  }
};

// Captures a list of MemberPointers.
template <typename... MemberPointers>
struct MemberList {
  using Members = std::tuple<MemberPointers...>;

  enum : std::size_t { Count = sizeof...(MemberPointers) };

  template <std::size_t Index>
  using At = typename std::tuple_element<Index, Members>::type;

  template <typename Class, typename T>
  static constexpr bool IsLastMember(const Class& instance, T Class::*member) {
    return IsLastMember(instance, member, Index<Count>{});
  }

 private:
  template <typename Class, typename T>
  static constexpr bool IsLastMember(const Class& /*instance*/,
                                     T Class::* /*member*/, Index<0>) {
    return true;
  }

  template <typename Class, typename T, std::size_t index>
  static constexpr bool IsLastMember(const Class& instance, T Class::*member,
                                     Index<index>) {
    const bool after = At<index - 1>::GreaterEqual(instance, member);
    return after && IsLastMember(instance, member, Index<index - 1>{});
  }
};

// Utility to retrieve a traits type that defines a MemberList for type T using
// ADL. The macros NOP_STRUCTURE, NOP_EXTERNAL_STRUCTURE, and
// NOP_EXTERNAL_TEMPLATE define the appropriate traits type and a defintion of
// NOP__GetExternalMemberTraits that this utility finds using ADL.
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

// Determines whether type T has a nested type named NOP__VALUE of
// template type MemberList.
template <typename T, typename = void>
struct IsValueWrapper {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__VALUE*) {
    return IsTemplateBaseOf<MemberList, typename U::NOP__VALUE>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };
};

// Enable utilities for value wrapper predicates.
template <typename T, typename ReturnType = void>
using EnableIfIsValueWrapper =
    typename std::enable_if<IsValueWrapper<T>::value, ReturnType>::type;
template <typename T, typename ReturnType = void>
using EnableIfIsNotValueWrapper =
    typename std::enable_if<!IsValueWrapper<T>::value, ReturnType>::type;

// Traits type that retrieves the internal MemberList and Pointer associated
// with type T.
template <typename T, typename = void>
struct ValueWrapperTraits;
template <typename T>
struct ValueWrapperTraits<T, EnableIfIsValueWrapper<T>> {
  using MemberList = typename T::NOP__VALUE;
  using Pointer = typename MemberList::template At<0>;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_DETAIL_MEMBER_POINTER_H_
