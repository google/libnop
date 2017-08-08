#ifndef LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
#define LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_

#include <memory>

#include <nop/base/encoding.h>
#include <nop/status.h>

namespace nop {

//
// Serializer and Deserializer template types provide the basic interface for
// writing and reading C++ types to a writer or reader class. There are several
// types of specializations of Serializer and Deserializer: those that contain
// an internal instance of the writer or reader and those that wrap a pointer or
// unique pointer to an external writer or reader.
//
// Example of instantiating a Serializer with an internal Writer:
//
//   Serializer<StreamWriter<std::stringstream>> serializer;
//   auto status = serializer.Write(data_type);
//
// Example of instantiating a Serializer with an external Writer:
//
//   using Writer = StreamWriter<std::stringstream>;
//   Writer stream_writer;
//   Serializer<Writer> serializer{&stream_writer};
//   auto status = serializer.Write(data_type);
//
// Which specialization to use depends on the situation and whether the writer
// or reader will be used in different contexts or only for serialization /
// deserialization tasks.
//

// Implementation of Write method common to all Serializer specializations.
struct SerializerCommon {
  template <typename T, typename Writer>
  static Status<void> Write(const T& value, Writer* writer) {
    // Determine how much space to prepare the writer for.
    const std::size_t size_bytes = Encoding<T>::Size(value);

    // Prepare the writer for the serialized data.
    auto status = writer->Prepare(size_bytes);
    if (!status)
      return status;

    // Serialize the data to the writer.
    return Encoding<T>::Write(value, writer);
  }
};

// Serializer with internal instance of Writer.
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
    return SerializerCommon::Write(value, &writer_);
  }

  const Writer& writer() const { return writer_; }
  Writer& writer() { return writer_; }
  Writer&& take() { return std::move(writer_); }

 private:
  Writer writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

// Serializer that wraps a pointer to Writer.
template <typename Writer>
class Serializer<Writer*> {
 public:
  Serializer() : writer_{nullptr} {}
  Serializer(Writer* writer) : writer_{writer} {}
  Serializer(const Serializer&) = default;
  Serializer& operator=(const Serializer&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, writer_);
  }

  const Writer& writer() const { return *writer_; }
  Writer& writer() { return *writer_; }

 private:
  Writer* writer_;
};

// Serializer that wraps a unique pointer to Writer.
template <typename Writer>
class Serializer<std::unique_ptr<Writer>> {
 public:
  Serializer() = default;
  Serializer(std::unique_ptr<Writer> writer) : writer_{std::move(writer)} {}
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
    return SerializerCommon::Write(value, writer_.get());
  }

  const Writer& writer() const { return *writer_; }
  Writer& writer() { return *writer_; }

 private:
  std::unique_ptr<Writer> writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

// Deserializer that wraps an internal instance of Reader.
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

// Deserializer that wraps a pointer to Reader.
template <typename Reader>
class Deserializer<Reader*> {
 public:
  Deserializer() : reader_{nullptr} {}
  Deserializer(Reader* reader) : reader_{reader} {}
  Deserializer(const Deserializer&) = default;
  Deserializer& operator=(const Deserializer&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_);
  }

  const Reader& reader() const { return *reader_; }
  Reader& reader() { return *reader_; }

 private:
  Reader* reader_;
};

// Deserializer that wraps a unique pointer to reader.
template <typename Reader>
class Deserializer<std::unique_ptr<Reader>> {
 public:
  Deserializer() = default;
  Deserializer(std::unique_ptr<Reader> reader) : reader_{std::move(reader)} {}
  Deserializer(Deserializer&&) = default;
  Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_.get());
  }

  const Reader& reader() const { return *reader_; }
  Reader& reader() { return *reader_; }

 private:
  std::unique_ptr<Reader> reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
