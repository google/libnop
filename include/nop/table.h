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

#ifndef LIBNOP_INCLUDE_NOP_TABLE_H_
#define LIBNOP_INCLUDE_NOP_TABLE_H_

#include <type_traits>

#include <nop/base/macros.h>
#include <nop/structure.h>
#include <nop/types/optional.h>
#include <nop/utility/sip_hash.h>

namespace nop {

//
// Tables are bi-directional binary compatible structures that support
// serializing and deserializing data from different versions of the same type.
// Use a table type when maintaining compatability between different versions of
// serialized data is important to the application. However, consider that every
// non-empty table entry cost at least two bytes more than the underlying type
// encoding.
//
// Users define tables using classes or structures with members of type
// Entry<T, Id>. Entry<T, Id> is similar to Optional<T> in that an entry may
// either be empty or contain a value of type T. Entries that are empty are not
// encoded during serialization to save space. Programs using tables should
// handle empty values in a sensible way, ensuring that missing entries in older
// data are handled gracefully.
//
// Example of a simple table definition:
//
// struct MyTable {
//   Entry<Address, 0> address;
//   Entry<PhoneNumber, 1> phone_number;
//   NOP_TABLE(MyTable, address, phone_number);
// };
//
// Example of handling empty values:
//
// MyTable my_table;
// auto status = serializer.Read(&my_table);
// if (!status)
//   return status;
//
// if (my_table.address)
//   HandleAddress(my_table.address.get());
// if (my_table.phone_number)
//   HandlePhoneNumber(my_table.phone_number.get());
//
// Table entries may be public, private, or protected, depending on how the
// table type will be used. If the entries are non-public, call NOP_TABLE*() in
// a private section to avoid exposing member pointers to arbitrary code.
//
// Use the following rules to maximize compatability between different versions
// of a table type:
//   1. Always use unique values for Id when adding an entry to a table. Never
//      reuse a previously used Id in the same table.
//   2. When deprecating an entry use the DeletedEntry type instead of deleting
//      the entry entirely to document the deprecation and prevent resuse of an
//      old entry id.
//   3. Never change the Id for an entry. Doing so will break compatibility with
//      older versions of serialized data.
//   4. Never change the namespace hash or namespace string passed as the first
//      argument to the macros NOP_TABLE_HASH / NOP_TABLE_NS. These values are
//      used for sanity checking during deserialization. Changing these
//      arguments will break compatibility with older versions of serialized
//      data.
//

// Type tags to define used and deprecated entries.
struct ActiveEntry {};
struct DeletedEntry {};

// Base type of table entries.
template <typename, std::uint64_t, typename = ActiveEntry>
struct Entry;

// Specialization of Entry for active/used entries. Each non-empty, active entry
// is encoded during serialization.
template <typename T, std::uint64_t Id_>
struct Entry<T, Id_, ActiveEntry> : Optional<T> {
  enum : std::uint64_t { Id = Id_ };
  using Optional<T>::Optional;
  using Optional<T>::operator=;
};

// Specialization of Entry for deleted/deprecated entries. These entries are
// always empty and are never encoded. When encountered during deserialization
// these entries are ignored.
template <typename T, std::uint64_t Id_>
struct Entry<T, Id_, DeletedEntry> {
  enum : std::uint64_t { Id = Id_ };

  bool empty() const { return true; }
  explicit operator bool() const { return !empty(); }
  void clear() {}
};

// Defines a table type, its namespace hash, and its members. This macro must be
// invoked once within a table struct or class to inform the serialization
// engine about the table members and hash value. The macro befriends several
// key classes and defines an internal type named NOP__ENTRIES that describes
// the table type's Entry<T, Id> members to the engine.
#define NOP_TABLE_HASH(hash, type, ... /*entries*/)             \
  template <typename, typename>                                 \
  friend struct ::nop::Encoding;                                \
  template <typename, typename>                                 \
  friend struct ::nop::HasEntryList;                            \
  template <typename>                                           \
  friend struct ::nop::EntryListTraits;                         \
  using NOP__ENTRIES = ::nop::EntryList<::nop::HashValue<hash>, \
                                        _NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Similar to NOP_TABLE_HASH except that the namespace hash is computed from a
// compile-time hash of the given string literal that defines the namespace of
// the table.
#define NOP_TABLE_NS(string_name, type, ... /*entries*/)                    \
  NOP_TABLE_HASH(::nop::SipHash::Compute(string_name, ::nop::kNopTableKey0, \
                                         ::nop::kNopTableKey1),             \
                 type, __VA_ARGS__)

// Similar to NOP_TABLE_HASH except that the namespace hash is set to zero,
// which has a compact encoding compared to average 64bit hash values. This
// saves space in the encoding when namespace checks are not desired.
#define NOP_TABLE(type, ... /*entries*/) NOP_TABLE_HASH(0, type, __VA_ARGS__)

// Determines whether two entries have the same id.
template <typename A, typename B>
struct SameEntryId : std::integral_constant<bool, A::Id == B::Id> {};

// Similar to MemberList used for serializable structures/classes. This type
// also records the hash of the table name for sanity checking during
// deserialization.
template <typename HashValue, typename... MemberPointers>
struct EntryList : MemberList<MemberPointers...> {
  static_assert(IsUnique<SameEntryId, typename MemberPointers::Type...>::value,
                "Entry ids must be unique.");
  enum : std::uint64_t { Hash = HashValue::Value };
};

// Work around access check bug in GCC. Keeping original code here to document
// the desired behavior. Bug filed with GCC:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82478
#if 0
// Determines whether the given type T has a nested type named NOP__ENTRIES that
// is a subtype of EntryList. This type evaluates to std::true_type for a
// properly defined table type, std::false_type otherwise.
template <typename, typename = void>
struct HasEntryList : std::false_type {};
template <typename T>
struct HasEntryList<T, Void<typename T::NOP__ENTRIES>>
    : std::integral_constant<
          bool, IsTemplateBaseOf<EntryList, typename T::NOP__ENTRIES>::value> {
};
#else
// Determines whether the given type T has a nested type named NOP__ENTRIES that
// is a subtype of EntryList.
template <typename T, typename = void>
struct HasEntryList {
 private:
  template <typename U>
  static constexpr bool Test(const typename U::NOP__ENTRIES*) {
    return IsTemplateBaseOf<EntryList, typename U::NOP__ENTRIES>::value;
  }
  template <typename U>
  static constexpr bool Test(...) {
    return false;
  }

 public:
  enum : bool { value = Test<T>(0) };
};
#endif

// Enable if HasEntryType<T> evaluates to true.
template <typename T>
using EnableIfHasEntryList =
    typename std::enable_if<HasEntryList<T>::value>::type;

// Traits type that retrieves the NOP__ENTRIES traits type for the given type T.
template <typename T>
struct EntryListTraits {
  using EntryList = typename T::NOP__ENTRIES;
};

// SipHash keys used to compute the table hash of the given table name string.
enum : std::uint64_t {
  kNopTableKey0 = 0xbaadf00ddeadbeef,
  kNopTableKey1 = 0x0123456789abcdef,
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TABLE_H_
