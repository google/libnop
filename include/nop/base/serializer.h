#ifndef LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
#define LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_

#include <nop/base/encoding.h>
#include <nop/status.h>

namespace nop {

template <typename Writer>
struct Serializer {
  Writer* writer;

  // Returns the encoded size of |value| in bytes.
  template <typename T>
  std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  Status<void> Write(const T& value) {
    // Determine how much space to prepare the writer for.
    const std::size_t size_bytes = GetSize(value);

    // Prepare the writer for the serialized data.
    auto status = writer->Prepare(size_bytes);
    if (!status)
      return status;

    // Serialize the data to the writer.
    return Encoding<T>::Write(value, writer);
  }
};

template <typename Reader>
struct Deserializer {
  Reader* reader;

  // Deserializes the data from the reader.
  template <typename T>
  Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
