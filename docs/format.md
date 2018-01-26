# Binary Format

Libnop uses a self-describing binary format to encode data. The format is
informed by other binary formats such as MessagePack and ProtoBufs.

## Goals

The libnop binary format has the following goals:

  * Provide a reasonably efficient representation of data that balances
    compactness, encoder complexity, and execution time.
  * Enable progressive validation of data during decode.
  * Enable examination of the structure of the encoded data without full
    knowledge of the original data types.
  * Provide a concise, flexible set of data representation strategies to cover
    a wide range of current and future needs.

## Structure

The format of the binary encoding follows a consistent prefix-payload structure
throughout. Each encoding in the format starts with a single-byte prefix
followed by zero or more bytes or a sub-element with its own prefix and payload.
This structure enables the decoder to progressively validate the data as it
reads the binary stream, and makes it possible to parse the encoded stream
without the high-level protocol definitions, a feature shared with formats like
JSON and MessagePack.

### Notation in Diagrams

```
One byte:
+--------+    +--------+    +--------+
|        | OR | value  | OR | label  |
+--------+    +--------+    +--------+

Variable number of bytes:
+---//---+
|        |
+---//---+

Variable number of objects:
+~~~~~~~~+
|        |
+~~~~~~~~+

Specific object or class:
+========+
| CLASS  |
+========+
```

### Prefix Definitions

The following table describes the encoding prefix values defined by the format:

Function        | Prefix | Binary   | Hex         | Description
--------------- | ------ | -------- | ----------- | -----------
positive fixint | POS    | 0xxxxxxx | 0x00 - 0x7f | Positive integer with embedded value in the range [0, 127].
false           | F      | 00000000 | 0x00        | False value. Alias of positive fixint 0.
true            | T      | 00000001 | 0x01        | True value. Alias of positive fixint 1.
uint8           | U8     | 10000000 | 0x80        | 8-bit unsigned integer.
uint16          | U16    | 10000001 | 0x81        | 16-bit unsigned integer.
uint32          | U32    | 10000010 | 0x82        | 32-bit unsigned integer.
uint64          | U64    | 10000011 | 0x83        | 64-bit unsigned integer.
int8            | I8     | 10000100 | 0x84        | 8-bit signed integer.
int16           | I16    | 10000101 | 0x85        | 16-bit signed integer.
int32           | I32    | 10000110 | 0x86        | 32-bit signed integer.
int64           | I64    | 10000111 | 0x87        | 64-bit signed integer.
float32         | F32    | 10001000 | 0x88        | single-precision floating point.
float64         | F64    | 10001001 | 0x89        | double-precision floating point.
reserved        |        | -------- | 0x8a - 0xb4 | Reserved for future use.
table           | TAB    | 10110101 | 0xb5        | Collection of N optional id/object blobs.
error           | ERR    | 10110110 | 0xb6        | Either an object or a non-zero integer error code.
handle          | HND    | 10110111 | 0xb7        | An integer index to an out-of-band resource.
variant         | VAR    | 10111000 | 0xb8        | Union type containing one of a number of objects or empty.
structure       | STU    | 10111001 | 0xb9        | Fixed-size collection of N objects.
array           | ARY    | 10111010 | 0xba        | Variable-sized collection of objects.
map             | MAP    | 10111011 | 0xbb        | Variable-sized collection of key/value (object/object) pairs.
binary          | BIN    | 10111100 | 0xbc        | Variable-sized binary blob.
string          | STR    | 10111101 | 0xbd        | Variable-sized string.
nil             | NIL    | 10111110 | 0xbe        | Nil / empty / none.
extension       | EXT    | 10111111 | 0xbf        | Extension type with unsigned integer extension code.
negative fixint | NEG    | 11xxxxxx | 0xc0 - 0xff | Negative integer with embedded value in the range of [-64, -1].

### Integer Encoding Class

The most fundamental encoding classes is the integer class: most other encodings
depend on the integer class as a sub-element. For example, containers use
integer encodings to specify the number of elements to follow.

The libnop integer representation strategy is very similar to that used by
MessagePack. Integer values are divided into signed and unsigned varieties. Each
variety has multiple encodings to save space when values fall within certain
ranges. For an integer type of a particular size, any encoding of equal or
smaller range is allowed; an encoding of a larger range is not allowed, even if
the encoded value is technically within range.

The *varint* encoding strategy used by ProtoBufs was considered for libnop
however, this strategy is not used as it complicates the self-describing
properties of the format and is more difficult for humans to read when
examining binary streams. Instead, the libnop format trades off the small space
savings provided by *varint* for better readability and safety.

#### Endianness

The integer encoding used by libnop is always **little-endian**. The reason for
this choice is that most modern architectures are little-endian. Even though
endianness conversion is relatively inexpensive it is an unnecessary step that
can be avoided on the majority of processors in use today.

#### Signed Integers

Signed integers are always stored in two's complement format.

```
Positive fixint stores a 7-bit positive integer in the range [0, 127].
      +--------+
POS = |0XXXXXXX|
      +--------+

Negative fixint stores a 6-bit negative integer in the range [-64, -1].
      +--------+
NEG = |11XXXXXX|
      +--------+

Taken together the single-byte signed fixint covers the range [-64, 127].

All other prefix bytes in the libnop format fall in the range covered by 10XXXXXX.

Signed 8-bit integer in the range [-128, 127]
      +--------+--------+
I8  = |  0x84  | BYTE 0 |
      +--------+--------+

Signed 16-bit integer in the range [-32768, 32767].
      +--------+--------+--------+
I16 = |  0x85  | BYTE 0 | BYTE 1 |
      +--------+--------+--------+

Signed 32-bit integer in the range [-2147483648, 2147483647].
      +--------+--------+--------+--------+--------+
I32 = |  0x86  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 |
      +--------+--------+--------+--------+--------+

Signed 64-bit integer in the range [-9223372036854775808, 9223372036854775807].
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
I64 = |  0x87  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 | BYTE 4 | BYTE 5 | BYTE 6 | BYTE 7 |
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
```

#### Unsigned Integers

Unsigned integers are always stored in direct binary magnitude format.

```
Positive fixint stores a 7-bit positive integer in the range [0, 127].
      +--------+
POS = |0XXXXXXX|
      +--------+

Signed 8-bit integer in the range [0, 255]
      +--------+--------+
U8  = |  0x80  | BYTE 0 |
      +--------+--------+

Signed 16-bit integer in the range [0, 65535].
      +--------+--------+--------+
U16 = |  0x81  | BYTE 0 | BYTE 1 |
      +--------+--------+--------+

Signed 32-bit integer in the range [0, 4294967295].
      +--------+--------+--------+--------+--------+
U32 = |  0x82  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 |
      +--------+--------+--------+--------+--------+

Signed 64-bit integer in the range [0, 18446744073709551616].
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
U64 = |  0x83  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 | BYTE 4 | BYTE 5 | BYTE 6 | BYTE 7 |
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
```

#### Integer Class Labels

In the following sections other encoding formats use the integer class for a
variety of purposes, such as encoding lengths, ids, hashes, and etc... The
format diagrams specify which subset of the integer class is valid using the
following notation:

  * INT8 - any signed integer 8-bits or smaller: POS, NEG, and I8
  * INT16 - any signed integer 16-bits or smaller: POS, NEG, I8, and I16
  * INT32 - any signed integer 32-bits or smaller: POS, NEG, I8, I16, and I32
  * INT64 - any signed integer 64-bits or smaller: POS, NEG, I8, I16, I32, and I64

Unsigned integer classes are labeled similarly:

  * UINT8 - POS and U8
  * UINT16 - POS, U8, and U16
  * UINT32 - POS, U8, U16, and U32
  * UINT64 - POS, U8, U16, U32, and U64

References to these class labels in format diagrams use the following form:

```
+========+    +========+
|  INT8  | OR | INT16  | OR etc...
+========+    +========+
```

### Floating Point Encoding Class

Floating point values are encoded directly: single-precision floats are stored
in F32 format and double-precision floats are stored in F64 format. No attempt
is made to handle variable-precision encodings.

Endianness of floating point representations is not specified by the relevant
standards. Modern x86 and ARM (but not all historical ARM) architectures use
little-endian floating point formats. See this Wikipedia entry for more details:
https://en.wikipedia.org/wiki/Endianness#Floating_point

Floating point encoding is supported for use cases where the same or similar
machines produce and consume the encoded data (i.e. internal IPC, file storage,
etc...). This library does not attempt to address cross-platform floating point
compatibility. In cases where cross-platform compatibility is important using
a fixed point intermediate representation is recommended.

```
Single precision floating point.
      +--------+--------+--------+--------+--------+
U32 = |  0x88  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 |
      +--------+--------+--------+--------+--------+

Double precision floating point.
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
U64 = |  0x89  | BYTE 0 | BYTE 1 | BYTE 2 | BYTE 3 | BYTE 4 | BYTE 5 | BYTE 6 | BYTE 7 |
      +--------+--------+--------+--------+--------+--------+--------+--------+--------+
```

### Array Container

The array container is a sized collection of values. The values may be either
homogeneous or heterogeneous types however, the validity of a particular type
depends on the container type specified by the high-level protocol definition.
For example, a protocol definition that uses `std::array<T, N>` may only have
wire format elements with valid encodings of type T, while a protocol
definition that uses `std::tuple<Ts...>` may have wire format elements with a
valid encoding of each corresponding element of `Ts`.

Note that the array container type is not used for strictly homogeneous arrays
of integral types (i.e. `std::vector<int>`, `std::array<short, N>`, etc...).
Integral arrays are encoded with the binary container type to avoid encoding
individual elements, saving time and space in larger arrays in some cases.

```
Array container:

N    = number of entries

                /  N   \
      +--------+========+~~~~~~~~~~~+
ARY = |  0xba  | UINT64 | N ENTRIES |
      +--------+========+~~~~~~~~~~~+
```

### Binary Container

The binary container is a sized byte string. This container may be used to
encapsulate opaque binary data or represent arrays of integral types.

As noted in the [array container](#array-container) section, homogeneous arrays
of integral types are encoded in the binary container format. The size of an
integral array is always encoded in bytes and must be an even multiple of the
integral element size. Each element is stored in direct binary representation
(not encoded in an integer class) in little-endian format.

```
Binary container (general):

N     = number of bytes

                /  N   \
      +--------+========+---//----+
BIN = |  0xbc  | UINT64 | N BYTES |
      +--------+========+---//----+

Binary container (integral array):

COUNT = number of integral elements
SIZE  = integral element size in bytes
N     = number of bytes, where N / SIZE == COUNT and N % SIZE == 0

                /  N   \
      +--------+========+~~~~~~~~~~~~~~~~+
BIN = |  0xbc  | UINT64 | COUNT INTEGERS |
      +--------+========+~~~~~~~~~~~~~~~~+
```

### String

The string type is a sized byte string. It is nearly identical the binary
container type, but uses a different prefix to provide a hint that the data
should be interpreted as text. This is helpful in situations where the type
information is not otherwise available, such as when analyzing the raw
self-describing data stream without the high-level protocol definitions.

The encoding of the text data is not specified and is up to the use case to
define and enforce. The size of the string must always be encoded in terms of
bytes, even when using wide character types, similar to how integer arrays are
encoded in the [binary container](#binary-container) format.

```
String (general):

N     = number of bytes

                /  N   \
      +--------+========+---//----+
STR = |  0xbd  | UINT64 | N BYTES |
      +--------+========+---//----+

String (wide characters):

COUNT = number of wide characters
SIZE  = wide character size in bytes
N     = number of bytes, where N / SIZE == COUNT and N % SIZE == 0

                /  N   \
      +--------+========+~~~~~~~~~~~~~+
STR = |  0xbd  | UINT64 | COUNT CHARS |
      +--------+========+~~~~~~~~~~~~~+
```

### Map Container

The map container is a sized collection of key/value pairs. There is no
restriction on key or value types at the binary format level however, the
high-level protocol definition may restrict which types are valid. For example,
`std::map<std::uint32_t, std::string>` only permits keys of the UINT32 integer
class and values of type STR.

```
Map container:

N    = number of entries

                /  N   \
      +--------+========+~~~~~~~~~~~+
MAP = |  0xbb  | UINT64 | N ENTRIES |
      +--------+========+~~~~~~~~~~~+

Map entry:

      +========+========+
ENT = |  KEY   | VALUE  |
      +========+========+
```

### Structure

The structure type is a fixed-size collection of objects. The high-level
protocol definition defines the size of the collection and the valid types for
each element. Structure types are not expected to evolve over time without
breaking binary compatibility with other versions of data.

```
Structure:

N    = number of elements

                /  N   \
      +--------+========+~~~~~~~~~~~~+
STU = |  0xb9  | UINT64 | N ELEMENTS |
      +--------+========+~~~~~~~~~~~~+
```

### Variant

The variant type is a union that may either be empty or hold one value. The
value may be any type in a set defined by the high-level protocol definition.

```
Variant:

I    = zero-based element type index or -1 for empty

                /  I   \
      +--------+========+===========+
VAR = |  0xb8  | INT64  | ELEMENT I |
      +--------+========+===========+

When the variant is empty the element value is NIL.

Empty variant:

      +--------+========+========+
VAR = |  0xb8  |   -1   |  NIL   |
      +--------+========+========+
```

### Handle

The handle type stores an integer handle/index to an out-of-band resource. This
can be used to represent file handles or other objects that are out-of-band with
the encoded byte stream.

```
Handle:

TYPE = value of handle-specific type used to validate this handle
REF  = reference number generated during serialization or -1 for empty

                         / REF  \
      +--------+========+========+
HND = |  0xb7  |  TYPE  | INT64  |
      +--------+========+========+
```

### Error

The error type stores an integer error code. This type is used in conjunction
with other types to form a composite result type that stores either a value or
an error. For example, `nop::Result<ErrorEnum, T>` specifies a type that may
store either a value of type T or an error value of type ErrorEnum.

```
Error:

ENUM = error code value of any integer class describing the error

      +--------+========+
ERR = |  0xb6  |  ENUM  |
      +--------+========+

Composite result type:

      +========+    +--------+========+
RES = | VALUE  | OR |  0xb6  |  ENUM  |
      +========+    +--------+========+
```

### Table Container

The table type is special collection of key/value pairs designed to support
bidirectional binary compatibility, similar in spirit to how ProtoBufs work.
The high-level protocol defines a table type as a collection of typed entries.
Each entry has a unique id (key) within the specific table and may either be
empty or contain a value of the type specified for the entry. In other words
table entries are optional type objects.

When a table is encoded the entries with values are written, skipping any entry
that is empty to save space. During decode only entries with recognized ids are
decoded, other entries with unrecognized ids are skipped. Because empty entries
are not encoded, newer table definitions that add entries can still decode data
generated from older definitions: the added entries are simply treated as
empty. Data generated by newer table definitions can be decoded by older table
definitions because entries with unrecognized ids are skipped. Likewise, newer
table definitions that remove entries also skip the now unrecognized deleted
entries in older data.

The first element following the table prefix is an unsigned encoded integer, up
to 64bits in size, that semi-uniquely identifies the table. This value provides
a simple validity check during decode. The libnop code generates this value at
compile-time by hashing a user-provided string. As a table format evolves over
time this value should remain the same to preserve compatibility with older data.

Entries may appear in any order in the encoded table however, a particular
entry may only appear once per encoded instance of the table. Duplicate ids in
an encoded table are treated as errors by the library.

In order to make parsing an entry with an unknown id easier the encoded bytes of
the entry's value are wrapped in a sized byte string. This makes it possible to
skip the value without parsing it at all. The sized byte string is permitted to
be larger than required for the encoded value to make it easier to handle types
that over estimate their size during encode. `nop::Handle` is an example of a
type that over estimates its size; this is necessary because the value of the
handle reference is determined after the size of the encoded handle is
estimated.

```
Table container:

HASH = semi-unique table id
N    = number of entries

                / HASH \ /  N   \
      +--------+========+========+~~~~~~~~~~~+
TAB = |  0xb5  | UINT64 | UINT64 | N ENTRIES |
      +--------+========+========+~~~~~~~~~~~+

Entry encoding:

ID   = unique entry id
N    = number of bytes in byte string, including padding bytes

       /  ID  \ /  N   \ /          N TOTAL BYTES          \
      +========+========+--------//-------+--------//-------+
ENT = | UINT64 | UINT64 |   VALUE BYTES   |  PADDING BYTES  |
      +========+========+--------//-------+--------//-------+
```

## Implementation

This section describes how libnop maps C++ types to the underlying binary format.

**TODO**
