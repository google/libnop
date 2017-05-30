#ifndef LIBNOP_INCLUDE_NOP_BASE_STRING_H_
#define LIBNOP_INCLUDE_NOP_BASE_STRING_H_

#include <string>

#include <nop/base/encoding.h>

namespace nop {

//
// std::string encoding format:
//
// +-----+---------+---//----+
// | STR | INT64:N | N BYTES |
// +-----+---------+---//----+
//

template <>
struct Encoding<std::string> : EncodingIO<std::string> {
  using Type = std::string;

  static constexpr EncodingByte Prefix(const std::string& /*value*/) {
    return EncodingByte::String;
  }

  static constexpr std::size_t Size(const std::string& value) {
    const std::size_t length_bytes =
        value.length() * sizeof(std::string::value_type);
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(length_bytes) + length_bytes;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::String;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/,
                                   const std::string& value, Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(value.length(), writer);
    if (!status)
      return status;

    return writer->WriteRaw(value.begin(), value.end());
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, std::string* value,
                                  Reader* reader) {
    std::uint64_t length_bytes;
    auto status = Encoding<std::uint64_t>::Read(&length_bytes, reader);
    if (!status)
      return status;
    else if (length_bytes % sizeof(std::string::value_type) != 0)
      return ErrorStatus(EIO);

    const std::uint64_t size = length_bytes / sizeof(std::string::value_type);

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous string sizes.
    status = reader->Ensure(size);
    if (!status)
      return status;

    value->resize(size);
    return reader->ReadRaw(&(*value)[0], &(*value)[size]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_STRING_H_