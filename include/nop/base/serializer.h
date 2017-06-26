#ifndef LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
#define LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_

#include <nop/base/encoding.h>
#include <nop/status.h>

namespace nop {

template <typename Writer>
class Serializer {
 public:
  template <typename... Args>
  Serializer(Args&&... args) : writer_{std::forward<Args>(args)...} {}

  Serializer(Serializer&&) = default;
  Serializer& operator=(Serializer&&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
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
    auto status = writer_.Prepare(size_bytes);
    if (!status)
      return status;

    // Serialize the data to the writer.
    return Encoding<T>::Write(value, &writer_);
  }

  const Writer& writer() const { return writer_; }
  Writer& writer() { return writer_; }
  Writer&& take() { return std::move(writer_); }

 private:
  Writer writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

template <typename Writer>
class Serializer<Writer*> {
 public:
  Serializer(Writer* writer) : writer_{writer} {}

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
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
    auto status = writer_->Prepare(size_bytes);
    if (!status)
      return status;

    // Serialize the data to the writer.
    return Encoding<T>::Write(value, writer_);
  }

  const Writer& writer() const { return *writer_; }
  Writer& writer() { return *writer_; }

 private:
  Writer* writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

template <typename Reader>
class Deserializer {
 public:
  template <typename... Args>
  Deserializer(Args&&... args) : reader_{std::forward<Args>(args)...} {}

  Deserializer(Deserializer&&) = default;
  Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  Status<void> Read(T* value) {
    return Encoding<T>::Read(value, &reader_);
  }

  const Reader& reader() const { return reader_; }
  Reader& reader() { return reader_; }
  Reader&& take() { return std::move(reader_); }

 private:
  Reader reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

template <typename Reader>
class Deserializer<Reader*> {
 public:
  Deserializer(Reader* reader) : reader_{reader} {}

  // Deserializes the data from the reader.
  template <typename T>
  Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_);
  }

  const Reader& reader() const { return *reader_; }
  Reader& reader() { return *reader_; }

 private:
  Reader* reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
