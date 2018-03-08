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
  static constexpr Status<void> Write(const T& value, Writer* writer) {
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
  constexpr Serializer(Args&&... args) : writer_{std::forward<Args>(args)...} {}

  constexpr Serializer(Serializer&&) = default;
  constexpr Serializer& operator=(Serializer&&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, &writer_);
  }

  constexpr const Writer& writer() const { return writer_; }
  constexpr Writer& writer() { return writer_; }
  constexpr Writer&& take() { return std::move(writer_); }

 private:
  Writer writer_;

  Serializer(const Serializer&) = delete;
  Serializer& operator=(const Serializer&) = delete;
};

// Serializer that wraps a pointer to Writer.
template <typename Writer>
class Serializer<Writer*> {
 public:
  constexpr Serializer() : writer_{nullptr} {}
  constexpr Serializer(Writer* writer) : writer_{writer} {}
  constexpr Serializer(const Serializer&) = default;
  constexpr Serializer& operator=(const Serializer&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, writer_);
  }

  constexpr const Writer& writer() const { return *writer_; }
  constexpr Writer& writer() { return *writer_; }

 private:
  Writer* writer_;
};

// Serializer that wraps a unique pointer to Writer.
template <typename Writer>
class Serializer<std::unique_ptr<Writer>> {
 public:
  constexpr Serializer() = default;
  constexpr Serializer(std::unique_ptr<Writer> writer)
      : writer_{std::move(writer)} {}
  constexpr Serializer(Serializer&&) = default;
  constexpr Serializer& operator=(Serializer&&) = default;

  // Returns the encoded size of |value| in bytes. This may be an over estimate
  // but must never be an under esitmate.
  template <typename T>
  constexpr std::size_t GetSize(const T& value) {
    return Encoding<T>::Size(value);
  }

  // Serializes |value| to the Writer.
  template <typename T>
  constexpr Status<void> Write(const T& value) {
    return SerializerCommon::Write(value, writer_.get());
  }

  constexpr const Writer& writer() const { return *writer_; }
  constexpr Writer& writer() { return *writer_; }

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
  constexpr Deserializer(Args&&... args)
      : reader_{std::forward<Args>(args)...} {}

  constexpr Deserializer(Deserializer&&) = default;
  constexpr Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, &reader_);
  }

  constexpr const Reader& reader() const { return reader_; }
  constexpr Reader& reader() { return reader_; }
  constexpr Reader&& take() { return std::move(reader_); }

 private:
  Reader reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

// Deserializer that wraps a pointer to Reader.
template <typename Reader>
class Deserializer<Reader*> {
 public:
  constexpr Deserializer() : reader_{nullptr} {}
  constexpr Deserializer(Reader* reader) : reader_{reader} {}
  constexpr Deserializer(const Deserializer&) = default;
  constexpr Deserializer& operator=(const Deserializer&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_);
  }

  constexpr const Reader& reader() const { return *reader_; }
  constexpr Reader& reader() { return *reader_; }

 private:
  Reader* reader_;
};

// Deserializer that wraps a unique pointer to reader.
template <typename Reader>
class Deserializer<std::unique_ptr<Reader>> {
 public:
  constexpr Deserializer() = default;
  constexpr Deserializer(std::unique_ptr<Reader> reader)
      : reader_{std::move(reader)} {}
  constexpr Deserializer(Deserializer&&) = default;
  constexpr Deserializer& operator=(Deserializer&&) = default;

  // Deserializes the data from the reader.
  template <typename T>
  constexpr Status<void> Read(T* value) {
    return Encoding<T>::Read(value, reader_.get());
  }

  constexpr const Reader& reader() const { return *reader_; }
  constexpr Reader& reader() { return *reader_; }

 private:
  std::unique_ptr<Reader> reader_;

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SERIALIZER_H_
