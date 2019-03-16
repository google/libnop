# Getting Started with C++ Native Object Protocols

The **Native Object Protocols** library (**libnop**) provides a simple toolbox
to make C++ serialization / deserialization convenient, flexible, and efficient.

This library gets its name from the use of C++ types (*native objects*) to
specify the structure of data (*protocols*). This framework essentially
*teaches* the C++ compiler how to understand and generate structured data
streams from the concrete types it encounters. The protocol of structured data
storage and interchange can be specified using primitive types, STL containers,
and structures with a few annotations without leaving the C++ language or using
additional tools, such as IDL code generators.

## Building With libnop

Building with **libnop** is as simple as adding the include directory to the
compiler include path and ensuring that C++14 or later is accepted. The
following example demonstrates building one of the examples with gcc on the
command line:

```
$ g++ -std=c++14 -Iinclude -o out/simple_protocol_example examples/simple_protocol.cpp
```

## Basic Usage

`nop::Serializer` and `nop::Deserializer` are the top-level types for reading
and writing data. These types provide the basic interface for converting C++
data types to and from serialized representations.

Here are the signatures of the `Read()` and `Write()` methods defined by these
classes:
```C++
namespace nop {

template <typename Reader>
template <typename T>
Status<void> Deserializer<Reader>::Read(T* value);

template <typename Writer>
template <typename T>
Status<void> Serializer<Writer>::Write(const T& value);

}  // namespace nop
```

This interface supports the following types:
  * Standard signed integer types: int8_t, int16_t, int32_t, and int64_t.
  * Regular signed integer types equivalent to the standard types:
    signed char, short, int, long, and long long.
  * Standard unsigned integer types: uint8_t, uint16_t, uint32_t, and uint64_t.
  * Regular unsigned integer types equivalent to the standard types:
    unsigned char, unsigned short, unsigned int, long, and unsigned long long.
  * char without signed/unsigned qualifiers.
  * bool.
  * enum and enum classes.
  * std::string.
  * C-style arrays with elements of any supported type.
  * std::array, std::pair, std::tuple, and std::vector with elements of any
    supported type.
  * std::map and std::unordered_map with keys and values of any supported type.
  * std::reference_wrapper<T> with T of any supported type.
  * nop::Optional<T> with T of any supported type.
  * nop::Result<ErrorEnum, T> with T of any supported type.
  * nop::Variant<Types...> with elements of any supported type.
  * nop::Handle and nop::UniqueHandle.
  * **User-defined structures** with members of any supported type.
  * **User-defined tables** with entries of any supported type.

Note that *any supported type* includes user-defined types and any level of
nesting. Circular references using `std::reference_wrapper` or `T&` are not
currently handled as the bookkeeping requires dynamic or scratch memory that
could impact fixed-memory use cases; this limitation may be addressed in the
future.

The following example demonstrates reading and writing some of these base types
to `std::stringstream`. The details of the Reader and Writer abstractions are
covered in [Reader/Writer Interfaces](#reader_writer-interfaces).

```C++
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <nop/serializer.h>
#include <nop/utility/die.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

namespace {

// Sends fatal errors to std::cerr.
auto Die() { return nop::Die(std::cerr); }

}  // anonymous namespace

int main(int, char**) {
  // Create a serializer around a std::stringstream.
  using Writer = nop::StreamWriter<std::stringstream>;
  nop::Serializer<Writer> serializer;

  // Write some data types to the stream.
  serializer.Write(10) || Die();
  serializer.Write(20.0f) || Die();
  serializer.Write(std::string{"The quick brown fox..."}) || Die();
  serializer.Write(std::array<std::string, 2>{{"abcdefg", "1234567"}}) || Die();
  serializer.Write(std::vector<std::uint64_t>{1, 2, 3, 4, 5, 6, 7}) || Die();
  serializer.Write(std::map<int, std::string>{{0, "abc"}, {1, "123"}}) || Die();

  // Create a deserializer around a std::stringstream with the serialized data.
  using Reader = nop::StreamReader<std::stringstream>;
  nop::Deserializer<Reader> deserializer{serializer.writer().take()};

  // Read some data types from the stream.
  int int_value;
  float float_value;
  std::string string_value;
  std::array<std::string, 2> array_value;
  std::vector<std::uint64_t> vector_value;
  std::map<int, std::string> map_value;

  deserializer.Read(&int_value) || Die();
  deserializer.Read(&float_value) || Die();
  deserializer.Read(&string_value) || Die();
  deserializer.Read(&array_value) || Die();
  deserializer.Read(&vector_value) || Die();
  deserializer.Read(&map_value) || Die();

  return 0;
}
```

## User-Defined Types

Writing sequences of primitive types and containers thereof is useful in simple
situations however, many situations benefit from further structure. In the
example above the order of calls to `Read()` and `Write()` and the types passed
to them implicitly define a *protocol*. In contrast, user-defined types provide
a means to explicitly specify the structure, or *protocol*, of the data stream.

There are three main classes of user-defined types: *structures*, *tables*, and
*value wrappers*.

Structures are space-efficient and appropriate for data that is relatively
fixed-format and whose direct structure is not expected to evolve over time. For
example, a three-component vector of floats that represents 3D positions,
velocities, and other 3D vectors is a stable data type that is appropriate for a
user-defined structure.

Tables are somewhat less efficient in space, execution time, and convenience but
offer bidirectional binary compatibility, allowing a user-defined table to add
and remove fields over time and still handle data from different versions of the
table.

Value wrappers provide a means to add policy to other serializable types without
adding additional overhead to the wire format. Value wrappers layer semantic
rules over other types by specifying constructors, operators, and accessors that
control value of the underling type.

Note that it is perfectly valid for user-defined structures to contain
user-defined table members and vice versa. A structure that contains tables
remains perfectly compatible with older data, even if its table members evolve,
as long as its own structure remains consistent. See the section on
[fungibility](#fungibility) for another way for structures to change while
remaining compatible.

### User-Defined Structures

A user-defined structure is defined by annotating a normal C/C++ struct or class
type. Annotation is performed using one of several macros that record
information about the user-defined type in the C++ type system. The annotation
is purely compile-time type information that does not affect the run-time memory
size of the user-defined type.

Once a user-defined structure or class is annotated the type is accepted by the
serialization API, and can be passed directly to `Read()` and `Write()` methods.
The type may also be nested in other types, such as STL containers and even
other user-defined types.

#### Internal Annotation

The easiest and most flexible way annotate a user-defined type is by using the
macro `NOP_STRUCTURE(type, ... /* members */)`. This macro must be invoked once
inside a struct or class and takes the type name followed by each member name as
arguments.

This macro defines a nested type, named `NOP__MEMBERS`, that stores compile-time
information about the structure's members. The macro also befriends the library
types responsible for serializing and deserializing the structure's members so
that private members can be accessed; it's a good idea to invoke the macro in
the private section of a class to avoid exposing the private members through the
nested type.

```C++
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nop/structure.h>

// A simple structure with internal annotation.
struct SimpleType {
  std::uint32_t foo;
  std::string bar;
  std::vector<std::uint32_t> baz;
  NOP_STRUCTURE(SimpleType, foo, bar, baz);
};

// Identical to SimpleType without the annotation.
struct UnannotatedSimpleType {
  std::uint32_t foo;
  std::string bar;
  std::vector<std::uint32_t> baz;
};

// Prove that annotation does not add to the run-time size of SimpleType.
static_assert(sizeof(SimpleType) == sizeof(UnannotatedSimpleType), "");

// A structure with a nested user-defined type.
struct NestedType {
  SimpleType bif;
  std::vector<SimpleType> fiz;
  NOP_STRUCTURE(NestedType, bif, fiz);
};

// A user-defined template type.
template <typename T, std::size_t N>
struct TemplateType {
  T foo;
  std::array<T, N> bar;

  // The abbreviated type name without parameters may be be passed as the type.
  NOP_STRUCTURE(TemplateType, foo, bar);
};

// A class with private members. Protocol types can use access controls and
// public methods to enforce policy on how the data may be used.
class Items {
 public:
  Items() = default;
  Items(std::vector<std::string> items) : items_{std::move(items)} {}
  Items(const Items&) = default;
  Items(Items&&) = default;
  Items& operator=(const Items&) = default;
  Items& operator=(Items&&) = default;

  const std::vector<std::string>& items() const { return items_; }

  bool empty() const { return items_.empty(); }
  explicit operator bool() const { return !empty(); }

 private:
  std::vector<std::string> items_;
  NOP_STRUCTURE(Items, items_);
};
```

#### External Annotation

In some situations it is desirable to separate the definition of the
user-defined type from the annotation. One example of this is when annotating a
type that belongs to a library that you can't or don't want to modify. A similar
situation arises when developing a C++ library with a C public API: some of the
public C structs may be useful to annotate for serialization by the internal C++
code. Separating the definition and annotation also helps avoid exposing the use
of **libnop** to external code.

Annotating an externally-defined type is performed using the
`NOP_EXTERNAL_STRUCTURE(type, ... /*members*/)` macro, which functions similarly
to `NOP_STRUCTURE()`, with some limitations. It is invoked outside of the struct
or class definition in global or namespace scope, usually in a different file
than the type being annotated.

Keep these points in mind when using an external annotation:
  * Only public members may be specified in an external annotation. There is no
    way to grant access to private or protected members without modifying the
    externally-defined structure or class.
  * The annotation macro must be invoked in the same namespace as the original
    externally-defined type. Calling the annotation on a type alias in another
    namespace will not work. One situation where this arises is when a C
    library is wrapped in a C++ API and the library aliases global types into a
    namespace for consistency or to avoid conflicts; in this case the global
    scope must be used for **libnop** to properly resolve the type.

Example header file with type to be externally annotated:
```C++
#ifndef LIBFOO_INCLUDE_PUBLIC_EXTERNAL_TYPE_H_
#define LIBFOO_INCLUDE_PUBLIC_EXTERNAL_TYPE_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Externally-defined structure in a public header.
typedef struct ExternalType {
  uint32_t a;
  uint8_t b[16];
  float c;
} ExternalType;

// Public function to operate on ExternalType.
int DoSomethingWithExternalType(const ExternalType* external_type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // LIBFOO_INCLUDE_PUBLIC_EXTERNAL_TYPE_H_
```

Example source file with external annotation:
```C++
#include <errno.h>

#include <nop/structure.h>
#include <nop/serializer.h>

#include "include/public/external_type.h"

// Annotate the externally-defined structure in the private C++ source file.
NOP_EXTERNAL_STRUCTURE(ExternalType, a, b, c);

// Uses a hypothetical foo::Writer to send the serialized data somewhere.
extern "C" int DoSomethingWithExternalType(const ExternalType* external_type) {
  nop::Serializer<foo::Writer> serializer;
  auto status = serializer.Write(*external_type);
  if (!status)
    return -EIO;

  // Do something with the data.
  status = serializer.writer().Send();
  if (!status)
    return -EIO;

  return 0;
}
```

#### Handling Template Types in External Annotations

There are two ways to annotate externally-defined template types:
  * Use `NOP_EXTERNAL_STRUCTURE()` to annotate each desired instantiation of the
    template type. This method can handle both type and non-type template
    arguments, but is less flexible because you must decide which arguments to
    use at annotation time.
  * Use `NOP_EXTERNAL_STRUCTURE()` to annotate the base template type name. This
    method is more flexible in that the external template type may be annotated
    once and then any instantiation is automatically recognized by the
    serializer at the site of use. The down side is that non-type template
    arguments are not supported.

Example header file with template type to be externally annotated:
```C++
#ifndef LIBFOO_INCLUDE_PUBLIC_SIMPLE_TYPE_H_
#define LIBFOO_INCLUDE_PUBLIC_SIMPLE_TYPE_H_

namespace foo {

template <typename T>
struct SimpleType {
  T data;
};

}  // namespace foo

#endif  // LIBFOO_INCLUDE_PUBLIC_SIMPLE_TYPE_H_
```

Example source file with external annotation:
```C++
#include <cstdint>
#include <string>

#include <nop/structure.h>

#include "include/public/simple_type.h"

namespace bar {

// Even though this is an alias in namespace bar the annotations must be in
// namespace foo. This is a consequence of how ADL resolves names, which
// external annotations use to handle cross-namespace lookup.
using foo::SimpleType;

}  // namespace bar

namespace foo {

// Example of annotating specific instantiations:
NOP_EXTERNAL_STRUCTURE(SimpleType<std::uint32_t>, data);
NOP_EXTERNAL_STRUCTURE(SimpleType<std::string>, data);

// Example of annotating the base template type name:
NOP_EXTERNAL_STRUCTURE(SimpleType, data);

// ... code to read and write SimpleType ...

}  // namespace foo
```

#### Logical Buffers

While `std::vector` is useful in many situations, sometimes it is desirable to
avoid dynamic memory allocation or constrain the size of the array or buffer,
while still supporting a variable number of elements. User-defined structures
support this by grouping a fixed-size array member with an appropriately wide
integer member.

The following example demonstrates the syntax:

```C++
struct Foo {
  std::uint32_t misc;
  std::uint8_t data[256];
  std::uint8_t size;

  // Members are grouped with parenthesis, array followed by size.
  NOP_STRUCTURE(Foo, misc, (data, size));
};

struct Vec3 {
  float x;
  float y;
  float z;
  NOP_STRUCTURE(Vec3, x, y, z);
};

struct Points {
  std::array<Vec3, 32> points;
  std::size_t num_points;
  NOP_STRUCTURE(Points, (points, num_points));
};

// It is possible to have more than one logical buffer in a structure, and to
// group members that are not adjacent in the structure definition.
struct TwoBuffers {
  std::uint8_t buffer_one[4096];
  std::uint8_t buffer_two[4096];
  std::size_t size_two;
  std::size_t size_one;
  NOP_STRUCTURE(TwoBuffers, (buffer_one, size_one), (buffer_two, size_two));
};
```

The members of a logical buffer are grouped using parenthesis in the member list
when annotating the user-defined structure. This may not be the most explicit
syntax, but it provides reasonable clarity without severely over complicating the
underlying macro processing rules.

This syntax is also supported for externally-defined types as well, which makes
it possible to serialize C-style buffer constructs embedded in external
structure definitions.

### User-Defined Tables

A table is a user-defined type that supports bidirectional binary
compatibility. This feature allows a table to evolve over time, either adding or
removing fields, while remaining compatible with data generated by both older
and newer versions of the same table type.

User-defined tables are defined similarly to user-defined structures: the main
difference is that all the members of a table must be instantiations of the
template type `nop::Entry<T, Id>` and must be annotated using the macro
`NOP_TABLE(type, ... /* entry members */)`.

There are some rules to follow in order to maintain compatibility with different
versions of data:
  * Every table entry must have a unique numeric id. This is specified in the
    `Id` template argument of `nop::Entry<T, Id>`. A compile-time check ensures
    that every entry within the same table has a unique id.
  * Once an id is used it must never be reused or re-purposed within the same
    table definition. Once a previously defined entry is removed its id should
    never be used again within the same table. As a best practice, instead of
    deleting an old entry mark it deleted using the optional template argument:
    `nop::Entry<T, Id, nop::DeletedEntry>`. This deactivates the entry, removes
    its storage, documents the deletion, and enables the compile-time check to
    continue to catch accidental reuse of the id.
  * Related to the previous point, the type of an entry may never be changed,
    unless it is to a fungible type. Changing the type of an entry to a
    different, incompatible type is effectively the same as reusing the id and
    breaks compatibility.
  * Unlike in user-defined structures, the entries of a table may be reordered
    for better readability.

Every entry in a table has an optional value: that is each entry may either
contain a value or be empty. This serves two purposes: First, empty entries do
not need to be written out during serialization, saving space in the byte
stream. Second, code must be written to handle each entry being empty, which
naturally addresses the situation where older data is missing newer fields or
newer table definitions remove entries. Additionally, entries with unrecognized
ids are skipped during deserialization. These properties provide full
bidirectional binary compatibility between different versions of a table
definition, provided that the above rules are followed.

The following is a simple example of defining a table type:
```C++
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <nop/base/table.h>
#include <nop/serializer.h>
#include <nop/utility/die.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

namespace {

// Sends fatal errors to std::cerr.
auto Die() { return nop::Die(std::cerr); }

struct SimpleCustomer {
  nop::Entry<std::string, 0> name;
  nop::Entry<std::string, 1> address;
  nop::Entry<std::vector<std::string>, 2> phone_numbers;
  NOP_TABLE(SimpleCustomer, name, address, phone_numbers);
};

} // anonymous namespace

int main(int, char**) {
  // Create a serializer around a std::stringstream.
  using Writer = nop::StreamWriter<std::stringstream>;
  nop::Serializer<Writer> serializer;

  serializer.Write(SimpleCustomer{
      "John Doe", "101 Somewhere St., City, State 55555", {{"408-555-1234"}}}) || Die();

  // Create a deserializer around a std::stringstream with the serialized data.
  using Reader = nop::StreamReader<std::stringstream>;
  nop::Deserializer<Reader> deserializer{serializer.writer().take()};

  SimpleCustomer customer;
  deserializer.Read(&customer) || Die();

  if (customer.name)
    std::cout << "Customer name: " << customer.name.get() << std::endl;

  if (customer.address)
    std::cout << "Customer address: " << customer.address.get() << std::endl;

  if (customer.phone_numbers) {
    std::cout << "Customer phone numbers: ";
    for (const auto& number : customer.phone_numbers.get())
      std::cout << number << " ";
    std::cout << std::endl;
  }

  return 0;
}
```

### User-Defined Value Wrappers

Values wrappers are user-defined types that add additional C++ policy to an
exsiting serializable type without affecting the wire format for the underlying
type. This policy can take the form of specific constructors, initializers,
operators and accessors that control what values the underlying type can take
and how the type interfaces with other code.

A value wrapper is defined by internally annotating a normal C++ struct or class
type using the `NOP_VALUE(type, member)` macro. This macro must be invoked once
inside a struct or class and takes the type name followed by the member to wrap.

The following are examples of value wrappers:
```C++
#include <nop/value.h>

// Simple template type that stores floating point values as fixed point
// integer with the specified number of fractional bits. This type has
// the same wire format as Integer.
template <typename Integer, std::size_t FractionalBits_>
class Fixed {
 public:
  enum : std::size_t {
    Bits = sizeof(Integer) * 8,
    FractionalBits = FractionalBits_,
    IntegralBits = Bits - FractionalBits
  };
  enum : std::size_t { Power = 1 << FractionalBits };

  static_assert(std::is_integral<Integer>::value, "");
  static_assert(FractionalBits < Bits, "");

  constexpr Fixed() = default;
  constexpr Fixed(const Fixed&) = default;
  constexpr Fixed(float f) { value_ = std::round(f * Power); }

  Fixed& operator=(const Fixed&) = default;
  Fixed& operator=(float f) { value_ = std::round(f * Power); }

  constexpr float float_value() const {
    return static_cast<float>(value_) / Power;
  }

  explicit constexpr operator float() const { return float_value(); }
  constexpr Integer value() const { return value_; }

 private:
  Integer value_;
  NOP_VALUE(Fixed, value_);
};

// A simple value wrapper around a logical buffer pair, providing a non-dynamic
// alternative to std::vector<T>. This type is fungible with std::vector<T>.
template <typename T, std::size_t Length>
struct ArrayWrapper {
  std::array<T, Length> data;
  std::size_t count;
  NOP_VALUE(ArrayWrapper, (data, count));
};
```

## Protocols, Validation, and Fungibility

This section takes a deeper look at how the library uses the C++ type system to
specify data interchange protocols, how data is validated during
deserialization, and a unique feature, called fungibility, that allows for
greater flexibility in how types are used in order to improve efficiency.

### Protocols

To help understand the way C++ types relate to defining *protocols*, consider
the following C++ value expression:

```C++
std::array<std::string, 4>{{"abcd", "1234", "ABCD", "2468"}}
```

When this expression is serialized the data looks like this:

| **BA** 04 **BD** 04 *61 62 63 64* **BD** 04 *31 32 33 34* **BD** 04 *41 42 43 44* **BD** 04 *32 34 36 38* |

This example demonstrates how the format of the data is related to the C++
types: here **BA** 04 denotes a four-element array, which corresponds to the
type `std::array<T, 4>`. Following this are four elements, each beginning with
**BD** 04, which denotes a four-byte string corresponding to the array element
type `std::string`.

### Validation

An important feature of the data interchange format is that it is
self-describing, meaning that the structure of the data is apparent from the
data itself. This enables the deserializer to validate the data stream as it is
deserialized using the same type-based protocol information that was used during
serialization. When the data doesn't match the expected shape of the data, the
deserializer rejects the stream as invalid.

Consider what happens when the same base type from the previous example is
passed to the deserializer:

```C++
std::array<std::string, 4> array_value;
deserializer.Read(&array_value);
```

The templates in the library generate a set of *expectations* for the structure
of a valid data stream. This *expected* pattern can be visualized like this:

| **BA** 04 **BD** X *X bytes* **BD** Y *Y bytes* **BD** Z *Z bytes* **BD** W *W bytes* |

This pattern requires an array of four elements, each of which must be a
variable-length string.

Note that it is possible for different type definitions to produce the same data
patterns. This property has useful advantages that are explored in the next
section.

### Fungibility

*Fungibility* refers to the ability to exchange one thing for another. In the
context of this library it refers to the ability to exchange types that specify
the same protocol. A simple example of fungible types are the array types
`std::array<int, 10>` and `int[10]`. Both of these types specify the same
protocol, and therefore we can substitute them for each other while retaining
compatibility with data produced by either.

This property can be used implicitly by simply using compatible types in
different places, such as in the following example:

```C++
using VariableBuffer = std::vector<std::uint8_t>;

// Writes a VariableBuffer buffer to the writer.
nop::Status<void> WriteData(Writer* writer, const VariableBuffer& data) {
  nop::Serializer<Writer*> serializer{writer};
  return serializer.Write(data);
}

// Writes a fixed-size array to the writer.
template <std::size_t N>
nop::Status<void> WriteData(Writer* writer, const std::uint8_t (&data)[N]) {
  nop::Serializer<Writer*> serializer{writer};
  return serializer.Write(data);
}

// Reads a VariableBuffer from the reader. Data generated by either overload of
// WriteData can be read.
nop::Status<void> ReadData(Reader* reader, VariableBuffer* data) {
  nop::Deserializer<Reader*> deserializer{reader};
  return deserializer.Read(data);
}
```

This example shows a simple situation where it is useful to substitute types. It
is inefficient and unnecessary to first copy the fixed array into a temporary
`std::vector` when the array size is large. Another benefit is avoiding the use
of dynamic memory that `std::vector` requires, which may be desirable in
situations that are memory-constrained or require very high performance.

#### nop::IsFungible

In very simple situations implicit fungibility of types may be sufficient
however, when incompatible types are used in different places, the break in
compatibility may only be noticed at run time, possibly long after the
problem is introduced. This is not a good situation, especially in large-scale,
high-impact projects with many contributors.

Fortunately, we can leverage the C++ compiler to validate type substitutions at
compile-time, ensuring that protocol violations are noticeable by breaking the
build. *libnop* provides several ways to invoke these checks that make it easy
to adapt to different requirements.

The basis of the compile-time type validation mechanism is the trait type
`nop::IsFungible<TypeA, TypeB>`, defined in `nop/traits/is_fungible.h`. This
trait compares two types and evaluates to true or false depending on whether
the two types may be mutually interchanged and preserve the protocol.

One way to use the compile-time check facility is by using `nop::IsFungible`
with `std::enable_if` to selectively enable or disable function overloads based
on the compatibility with a reference type that defines the base protocol. The
following illustrates this by modifying the previous example to take any
type compatible with the protocol type when reading or writing the data. This
supports a great deal of flexibility in how the functions are used.

```C++
#include <nop/traits/is_fungible.h>

// Define a type that is used as the standard "protocol" for this interchange.
using Buffer = std::vector<std::uint8_t>;

// Writes a type compatible with Buffer to the writer.
template <typename T, typename Enabled = nop::EnableIfFungible<Buffer, T>>
nop::Status<void> WriteData(Writer* writer, const T& data) {
  nop::Serializer<Writer*> serializer{writer};
  return serializer.Write(data);
}

// Reads a type compatible with Buffer from the reader.
template <typename T, typename Enabled = nop::EnableIfFungible<Buffer, T>>
nop::Status<void> ReadData(Reader* reader, T* data) {
  nop::Deserializer<Reader*> deserializer{reader};
  return deserializer.Read(data);
}
```

This version of the example uses `nop::EnableIfFungible<A, B, Return = void>`,
which is a utility that conveniently combines `std::enable_if` with
`nop::IsFungible<A, B>` to make enable expressions simpler and easier to read.
When an incompatible type is passed to either method the compiler rejects the
invocation with an error message about the enable expression.

#### nop::Protocol

Using enable expressions to validate protocols is useful when you want
flexibility in your own internal or external API. However, there are times where
performing validation inside a method or function is more useful or practical;
it would be inconvenient to have to write a template function every time a new
type substitution is needed. Fortunately **libnop** has a simple solution:
`nop::Protocol<ProtocolType>`.

`nop::Protocol` is a template type that provides a simple means of validating
compatibility between types at the point where the serializer or deserializer
are invoked to read or write data. The definition of this template type is
quite simple:

```C++
namespace nop {

// Implements a simple compile-time type-based protocol check. Overload
// resolution for Write/Read methods succeeds if the argument passed for
// serialization/deserialization is compatible with the protocol type
// ProtocolType.
template <typename ProtocolType>
struct Protocol {
  template <typename Serializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static Status<void> Write(Serializer* serializer, const T& value) {
    return serializer->Write(value);
  }

  template <typename Deserializer, typename T,
            typename Enable = EnableIfFungible<ProtocolType, T>>
  static Status<void> Read(Deserializer* deserializer, T* value) {
    return deserializer->Read(value);
  }
};

}  // namespace nop
```

The following example illustrates this type in action:

```C++
#include <nop/protocol.h>

using StringsProtocol = std::vector<std::string>;

nop::Status<void> WriteData(Writer* writer, const std::string& a,
                            const std::string& b, const std::string& c) {
  nop::Serializer<Writer*> serializer{writer};
  return nop::Protocol<StringsProtocol>::Write(&serializer, std::tie(a, b, c));
}
```

Here `std::tie()` produces a value expression of type
`std::tuple<const std::string&, const std::string&, const std::string&>` which
holds copies of the references passed as arguments. This this works because this
tuple type is fungible with `std::vector<std::string>`, and allows the
serializer to directly access the `std::string` objects passed as arguments
without any copying, which is much more efficient than creating a temporary
`std::vector<std::string>` and copying the three argument strings.

#### Fungible User-Defined Types

The examples in the previous sections introduced the concept of fungibility
using STL containers and primitives. User-defined types may also be fungible as
well. The condition for this is simple: if each member of one user-defined type
is fungible with each corresponding member of another user-defined type then
the user-defined types are considered to be fungible.

The following example illustrates this:
```C++
struct Foo {
  std::vector<std::uint8_t> buffer;
  NOP_STRUCTURE(Foo, buffer);
};

struct Bar {
  std::uint8_t data[128];
  std::size_t size;
  NOP_STRUCTURE(Bar, (data, size)); // Logical buffers are fungible too.
};

static_assert(nop::IsFungible<Foo, Bar>::value, "");

struct Baz {
  nop::Entry<Foo, 0> buffer_entry;
  NOP_TABLE(Baz, buffer_entry);
};

struct Bif {
  nop::Entry<Bar, 0> buffer_entry;
  NOP_TABLE(Bif, buffer_entry);
};

static_assert(nop::IsFungible<Baz, Biff>::value, "");
```

## Reader/Writer Interfaces

The `nop::Serializer` and `nop::Deserializer` types take a template parameter
that specifies the Reader/Writer type to handle storage and/or transmission of
the serialized data stream. This abstraction makes it easy to adapt the library
to different transport or storage requirements.

A few useful reader/writer types are shipped with the library, making it easy to
get started without any extra work. For more advanced situations it may be
necessary to write your own. See
[Writing Your Own Reader/Writer](#writing-your-own-reader_writer) for more
details.

The underlying reader or writer is available using the
`nop::Deserializer::reader()` and `nop::Serializer::writer()` methods. Aside
from the required reader/writer interface, these types may expose other methods
that are relevant to the specific reader or writer.

### StreamReader and StreamWriter

The `nop::StreamReader` and `nop::StreamWriter` types support STL streams. These
types take a single template parameter that specifies the stream type to use
internally.

The following example demonstrates basic instantiation using
`nop::StreamReader` and `nop::StreamReader` with `std::stringstream` and
`std::ostringstream`.

```C++
#include <iostream>
#include <sstream>

#include <nop/serializer.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

// Use default std::stringstream constructor.
nop::Serializer<nop::StreamWriter<std::stringstream>> serializer;

// Construct the internal std::ostringstream with an initial string in append mode.
nop::Serializer<nop::StreamWriter<std::ostringstream>> serializer{"Header", std::ios_base::ate};

// Construct by moving the internal std::stringstream from another instance.
nop::Deserializer<std::StreamReader<std::stringstream>> deserializer{serializer.writer().take()};
```

The underlying stream may be accessed by the `stream()` and `take()` methods of
`nop::StreamReader` and `nop::StreamWriter`.

### BufferReader and BufferWriter

`nop::BufferReader` and `nop::BufferWriter` support reading and writing raw byte
buffers. These types take a pointer to the external byte buffer and its size as
parameters. Many kinds of data transport can be built with these types by
layering buffer-based serialization over a transport layer.

The following example demonstrates instantiating a `nop::Serializer` and
`nop::Deserializer` around a plain C array using `nop::BufferWriter` and
`nop::BufferReader`:

```C++
#include <cstddef>
#include <cstdint>

#include <nop/serializer.h>
#include <nop/utility/buffer_reader.h>
#include <nop/utility/buffer_writer.h>


constexpr std::size_t kBufferSize = 1024;
std::uint8_t buffer[kBufferSize];

nop::Serializer<nop::BufferWriter> serializer{buffer, kBufferSize};
nop::Deserializer<nop::BufferReader> deserializer{buffer, kBufferSize};
```

The current number of bytes written to the buffer is available through the
`nop::BufferWriter::size()` method and the number of bytes remaining in the
buffer is available through the `nop::BufferReader::remaining()` method.

### Writing Your Own Reader/Writer

Building your own reader or writer type is straightforward: there are only four
required methods to implement. If handle support is required for out-of-band
objects then additional methods are required for each type of handle, or perhaps
a single template method that can receive all required handle types.

#### Basic Reader Interface

```C++
#include <cstddef>
#include <cstdint>

#include <nop/status.h>

class Reader {
 public:
  // Required methods:

  // Ensures that at least |size| bytes can be read before reaching the end the
  // input. Bounded readers should always implement this method to perform the
  // bounds check as an extra precaution against abusive data; this method is
  // used to guard against container/string lengths that would exceed the
  // available data. Unbounded readers, like nop::StreamReader, may
  // unconditionally return ErrorStatus::None at the cost of less security.
  //
  // Returns ErrorStatus::None if |size| bytes can be read from the reader.
  // Returns ErrorStatus::ReadLimitReached if the requested size would exceed
  // the available data.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Ensure(std::size_t size);

  // Reads a single byte from the input and returns it. This is a common
  // operation because of the prefix-oriented format. Breaking this out into a
  // separate overload improves code density on some platforms.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::ReadLimitReached if there are no more bytes left in
  // the input.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Read(std::uint8_t* byte);

  // Reads bytes into the buffer starting at |begin| until reaching |end|.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::ReadLimitReached if there are no more bytes left in
  // the input.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Read(void* begin, void* end);

  // Skips |padding_bytes| number of bytes in the input.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::ReadLimitReached if there are no more bytes left in
  // the input.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Skip(std::size_t padding_bytes);

  // Optional methods:

  // Returns a handle of type HandleType for the given |handle_reference|. This
  // is an optional method that is only necessary when deserializing nop::Handle
  // and related types. Implementations may either use single template method or
  // explicit overloads for the types of handles supported.
  //
  // Returns the handle on success.
  // Returns ErrorStatus::InvalidHandleReference if the given reference is not
  // valid. May return other errors particular to the reader implementation.
  template <typename HandleType>
  nop::Status<HandleType> GetHandle(HandleReference handle_reference);
};
```

#### Basic Writer Interface

```C++
#include <cstddef>
#include <cstdint>

#include <nop/status.h>

class Writer {
 public:
  // Required methods:

  // Ensures that at least |size| bytes can be written before reaching the end
  // of the output. This method may be used to allocate more buffer space or
  // other resource management tasks.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::WriteLimitReached if |size| would exceed the internal
  // limits of the writer.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Prepare(std::size_t size);

  // Writes a byte to the output. This is a common operation because of the
  // prefix-oriented format. Breaking this out into a separate overload
  // increases code density on some platforms.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::WriteLimitReached if no more bytes can be accepted to
  // the output.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Write(std::uint8_t byte);

  // Writes bytes from the buffer starting at |begin| until reaching |end|.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::WriteLimitReached if no more bytes can be accepted to
  // the output.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Write(const void* begin, const void* end);

  // Skips |padding_bytes| bytes in the output using |padding_value| as a
  // filler value.
  //
  // Returns ErrorStatus::None on success.
  // Returns ErrorStatus::WriteLimitReached if no more bytes can be accepted to
  // the output.
  // May return other errors particular to the reader implementation.
  nop::Status<void> Skip(std::size_t padding_bytes, std::uint8_t padding_value);

  // Optional methods:

  // Records a handle to a resource for out-of-band storage or transmission and
  // returns a HandleReference that may be serialized to retrieve the handle
  // during deserialization. This is an optional method that is only necessary
  // when serializing nop::Handle and related types. Implementations may use
  // either a template method or explicit overloads for the types of handles
  // supported.
  //
  // Returns a valid HandleReference on success.
  // Returns kEmptyHandleReference if |handle| is empty.
  // May return other errors particular to the reader implementation.
  template <typename HandleType>
  nop::Status<HandleReference> PushHandle(const HandleType& handle);
};
```

