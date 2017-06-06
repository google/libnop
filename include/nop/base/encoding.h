#ifndef LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_

#include <errno.h>

#include <cstdint>
#include <string>
#include <vector>

#include <nop/base/encoding_byte.h>
#include <nop/status.h>

namespace nop {

// Returns the size of the base encodings excluding extension payloads.
inline constexpr std::size_t BaseEncodingSize(EncodingByte prefix) {
  switch (prefix) {
    case EncodingByte::PositiveFixIntMin... EncodingByte::PositiveFixIntMax:
    case EncodingByte::NegativeFixIntMin... EncodingByte::NegativeFixIntMax:
    /* case EncodingByte::False ... EncodingByte::True: */
    case EncodingByte::Handle:
    case EncodingByte::Variant:
    case EncodingByte::Structure:
    case EncodingByte::Array:
    case EncodingByte::Map:
    case EncodingByte::Binary:
    case EncodingByte::String:
    case EncodingByte::Nil:
    case EncodingByte::Extension:
      return 1U;

    case EncodingByte::U8:
    case EncodingByte::I8:
      return 2U;

    case EncodingByte::U16:
    case EncodingByte::I16:
      return 3U;

    case EncodingByte::U32:
    case EncodingByte::I32:
    case EncodingByte::F32:
      return 5U;

    case EncodingByte::U64:
    case EncodingByte::I64:
    case EncodingByte::F64:
      return 9U;

    case EncodingByte::ReservedMin... EncodingByte::ReservedMax:
      return 0U;
  }
}

template <typename T, typename Enabled = void>
struct Encoding {
  // Generate a clear compiler error if there is no encoding specified for a
  // type passed to the serialization engine.
  static_assert(sizeof(T) != sizeof(T),
                "Encoding<T> must be specilaized for type T. Make sure to "
                "include the appropriate encoder header.");
};

// Implements general IO for encoding types. May also be mixed-in with an
// Encoding<T> specialization to provide uniform access to Read/Write through
// the specilization itself.
// Example:
//    template <>
//    struct Encoding<SomeType> : EncodingIO<SomeType> { ... };
//
//    ...
//
//    Encoding<SomeType>::Write(...);
//
template <typename T>
struct EncodingIO {
  template <typename Writer>
  static Status<void> Write(const T& value, Writer* writer) {
    EncodingByte prefix = Encoding<T>::Prefix(value);
    auto status = writer->Write(prefix);
    if (!status)
      return status;
    else
      return Encoding<T>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static Status<void> Read(T* value, Reader* reader) {
    EncodingByte prefix;
    auto status = reader->Read(&prefix);
    if (!status)
      return status;

    if (Encoding<T>::Match(prefix))
      return Encoding<T>::ReadPayload(prefix, value, reader);
    else
      return ErrorStatus(EIO);
  }
};

//
// bool encoding formats:
//
// +-------+
// | FALSE |
// +-------+
//
// +------+
// | TRUE |
// +------+
//

template <>
struct Encoding<bool> : EncodingIO<bool> {
  using Type = bool;

  static constexpr EncodingByte Prefix(bool value) {
    return value ? EncodingByte::True : EncodingByte::False;
  }

  static constexpr std::size_t Size(bool value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::True || prefix == EncodingByte::False;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, bool /*value*/,
                                   Writer* /*writer*/) {
    return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, bool* value,
                                  Reader* /*reader*/) {
    *value = static_cast<bool>(prefix);
    return {};
  }
};

//
// char encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// Type 'char' is a little bit special in that it is distinct from 'signed char'
// and 'unsigned char'. Treating it as an unsigned 8-bit value is safe for
// encoding purposes and nicely handles 7-bit ASCII encodings as POSFIXINT.
//

template <>
struct Encoding<char> : EncodingIO<char> {
  static constexpr EncodingByte Prefix(char value) {
    if (static_cast<std::uint8_t>(value) < (1U << 7))
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::U8;
  }

  static constexpr std::size_t Size(char value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           prefix == EncodingByte::U8;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, char value,
                                   Writer* writer) {
    if (prefix == EncodingByte::U8)
      return writer->template Write<std::uint8_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, char* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return reader->template Read<std::uint8_t>(value);
    } else {
      *value = static_cast<char>(prefix);
      return {};
    }
  }
};

//
// std::uint8_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//

template <>
struct Encoding<std::uint8_t> : EncodingIO<std::uint8_t> {
  static constexpr EncodingByte Prefix(std::uint8_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::U8;
  }

  static constexpr std::size_t Size(std::uint8_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           prefix == EncodingByte::U8;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::uint8_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::U8)
      return writer->template Write<std::uint8_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::uint8_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return reader->template Read<std::uint8_t>(value);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int8_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//

template <>
struct Encoding<std::int8_t> : EncodingIO<std::int8_t> {
  static constexpr EncodingByte Prefix(std::int8_t value) {
    if (value >= -32)
      return static_cast<EncodingByte>(value);
    else
      return EncodingByte::I8;
  }

  static constexpr std::size_t Size(std::int8_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return (prefix >= EncodingByte::PositiveFixIntMin &&
            prefix <= EncodingByte::PositiveFixIntMax) ||
           (prefix >= EncodingByte::NegativeFixIntMin &&
            prefix <= EncodingByte::NegativeFixIntMax) ||
           prefix == EncodingByte::I8;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::int8_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::I8)
      return writer->template Write<std::int8_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::int8_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return reader->template Read<std::int8_t>(value);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint16_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint16_t> : EncodingIO<std::uint16_t> {
  static constexpr EncodingByte Prefix(std::uint16_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1U << 8))
      return EncodingByte::U8;
    else
      return EncodingByte::U16;
  }

  static constexpr std::size_t Size(std::uint16_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint8_t>::Match(prefix) || prefix == EncodingByte::U16;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::uint16_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::U8)
      return writer->template Write<std::uint8_t>(value);
    else if (prefix == EncodingByte::U16)
      return writer->template Write<std::uint16_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::uint16_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return reader->template Read<std::uint8_t>(value);
    } else if (prefix == EncodingByte::U16) {
      return reader->template Read<std::uint16_t>(value);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int16_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int16_t> : EncodingIO<std::int16_t> {
  static constexpr EncodingByte Prefix(std::int16_t value) {
    if (value >= -32 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)
      return EncodingByte::I8;
    else
      return EncodingByte::I16;
  }

  static constexpr std::size_t Size(std::int16_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int8_t>::Match(prefix) || prefix == EncodingByte::I16;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::int16_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::I8)
      return writer->template Write<std::int8_t>(value);
    else if (prefix == EncodingByte::I16)
      return writer->template Write<std::int16_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::int16_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return reader->template Read<std::int8_t>(value);
    } else if (prefix == EncodingByte::I16) {
      return reader->template Read<std::int16_t>(value);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint32_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U32 | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint32_t> : EncodingIO<std::uint32_t> {
  static constexpr EncodingByte Prefix(std::uint32_t value) {
    if (value < (1U << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1U << 8))
      return EncodingByte::U8;
    else if (value < (1U << 16))
      return EncodingByte::U16;
    else
      return EncodingByte::U32;
  }

  static constexpr std::size_t Size(std::uint32_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint16_t>::Match(prefix) ||
           prefix == EncodingByte::U32;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::uint32_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::U8)
      return writer->template Write<std::uint8_t>(value);
    else if (prefix == EncodingByte::U16)
      return writer->template Write<std::uint16_t>(value);
    else if (prefix == EncodingByte::U32)
      return writer->template Write<std::uint32_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::uint32_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return reader->template Read<std::uint8_t>(value);
    } else if (prefix == EncodingByte::U16) {
      return reader->template Read<std::uint16_t>(value);
    } else if (prefix == EncodingByte::U32) {
      return reader->template Read<std::uint32_t>(value);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int32_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I32 | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int32_t> : EncodingIO<std::int32_t> {
  static constexpr EncodingByte Prefix(std::int32_t value) {
    if (value >= -32 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)
      return EncodingByte::I8;
    else if (value >= -32768 && value <= 32767)
      return EncodingByte::I16;
    else
      return EncodingByte::I32;
  }

  static constexpr std::size_t Size(std::int32_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int16_t>::Match(prefix) || prefix == EncodingByte::I32;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::int32_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::I8)
      return writer->template Write<std::int8_t>(value);
    else if (prefix == EncodingByte::I16)
      return writer->template Write<std::int16_t>(value);
    else if (prefix == EncodingByte::I32)
      return writer->template Write<std::int16_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::int32_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return reader->template Read<std::int8_t>(value);
    } else if (prefix == EncodingByte::I16) {
      return reader->template Read<std::int16_t>(value);
    } else if (prefix == EncodingByte::I32) {
      return reader->template Read<std::int32_t>(value);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// std::uint64_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +----+------+
// | U8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | U16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U32 | 4 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | U64 | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::uint64_t> : EncodingIO<std::uint64_t> {
  static constexpr EncodingByte Prefix(std::uint64_t value) {
    if (value < (1ULL << 7))
      return static_cast<EncodingByte>(value);
    else if (value < (1ULL << 8))
      return EncodingByte::U8;
    else if (value < (1ULL << 16))
      return EncodingByte::U16;
    else if (value < (1ULL << 32))
      return EncodingByte::U32;
    else
      return EncodingByte::U64;
  }

  static constexpr std::size_t Size(std::uint64_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::uint32_t>::Match(prefix) ||
           prefix == EncodingByte::U64;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::uint64_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::U8)
      return writer->template Write<std::uint8_t>(value);
    else if (prefix == EncodingByte::U16)
      return writer->template Write<std::uint16_t>(value);
    else if (prefix == EncodingByte::U32)
      return writer->template Write<std::uint32_t>(value);
    else if (prefix == EncodingByte::U64)
      return writer->template Write<std::uint64_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::uint64_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::U8) {
      return reader->template Read<std::uint8_t>(value);
    } else if (prefix == EncodingByte::U16) {
      return reader->template Read<std::uint16_t>(value);
    } else if (prefix == EncodingByte::U32) {
      return reader->template Read<std::uint32_t>(value);
    } else if (prefix == EncodingByte::U64) {
      return reader->template Read<std::uint64_t>(value);
    } else {
      *value = static_cast<std::uint8_t>(prefix);
      return {};
    }
  }
};

//
// std::int64_t encoding formats:
//
// +-----------+
// | POSFIXINT |
// +-----------+
//
// +-----------+
// | NEGFIXINT |
// +-----------+
//
// +----+------+
// | I8 | BYTE |
// +----+------+
//
// +-----+---//----+
// | I16 | 2 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I32 | 4 BYTES |
// +-----+---//----+
//
// +-----+---//----+
// | I64 | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<std::int64_t> : EncodingIO<std::int64_t> {
  static constexpr EncodingByte Prefix(std::int64_t value) {
    if (value >= -32 && value <= 127)
      return static_cast<EncodingByte>(value);
    else if (value >= -128 && value <= 127)  // Effectively [-128, -32).
      return EncodingByte::I8;
    else if (value >= -32768 && value <= 32767)
      return EncodingByte::I16;
    else if (value >= -2147483648 && value <= 2147483647)
      return EncodingByte::I32;
    else
      return EncodingByte::I64;
  }

  static constexpr std::size_t Size(std::int64_t value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<std::int32_t>::Match(prefix) || prefix == EncodingByte::I64;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, std::int64_t value,
                                   Writer* writer) {
    if (prefix == EncodingByte::I8)
      return writer->template Write<std::int8_t>(value);
    else if (prefix == EncodingByte::I16)
      return writer->template Write<std::int16_t>(value);
    else if (prefix == EncodingByte::I32)
      return writer->template Write<std::int32_t>(value);
    else if (prefix == EncodingByte::I64)
      return writer->template Write<std::int64_t>(value);
    else
      return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, std::int64_t* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::I8) {
      return reader->template Read<std::int8_t>(value);
    } else if (prefix == EncodingByte::I16) {
      return reader->template Read<std::int16_t>(value);
    } else if (prefix == EncodingByte::I32) {
      return reader->template Read<std::int32_t>(value);
    } else if (prefix == EncodingByte::I64) {
      return reader->template Read<std::int64_t>(value);
    } else {
      *value = static_cast<std::int8_t>(prefix);
      return {};
    }
  }
};

//
// float encoding format:
// +-----+---//----+
// | FLT | 4 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<float> : EncodingIO<float> {
  static constexpr EncodingByte Prefix(float /*value*/) {
    return EncodingByte::F32;
  }

  static constexpr std::size_t Size(float value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::F32;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, float value,
                                   Writer* writer) {
    return writer->Write(&value, &value + 1);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, float* value,
                                  Reader* reader) {
    return reader->Read(value, value + 1);
  }
};

//
// double encoding format:
// +-----+---//----+
// | DBL | 8 BYTES |
// +-----+---//----+
//

template <>
struct Encoding<double> : EncodingIO<double> {
  static constexpr EncodingByte Prefix(double /*value*/) {
    return EncodingByte::F64;
  }

  static constexpr std::size_t Size(double value) {
    return BaseEncodingSize(Prefix(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::F64;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, double value,
                                   Writer* writer) {
    return writer->Write(&value, &value + 1);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, double* value,
                                  Reader* reader) {
    return reader->Read(value, value + 1);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENCODING_H_
