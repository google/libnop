// Copyright 2017 The Native Object Protocols Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <array>
#include <iostream>
#include <sstream>
#include <vector>

#include <nop/protocol.h>
#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/status.h>
#include <nop/utility/die.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"
#include "string_to_hex.h"

using nop::Deserializer;
using nop::ErrorStatus;
using nop::Protocol;
using nop::Serializer;
using nop::Status;
using nop::StreamReader;
using nop::StreamWriter;
using nop::StringToHex;

//
// simple_protocol.cpp - Example of using compile-time protocol and fungible
// types.
//
// A protocol type is a C++ type that sepcifies the valid data format of a
// particular datum in a defined communication protocol.
//
// The protocol system supports the notion of fungible types: types that are
// fungible produce the same wire format and can be used interchangeably when
// calling the serialization engine. The class nop::Protocol provides a simple
// way to verify that code conforms to a type-safe protocol by checking that the
// types passed to its Read/Write methods are fungible with the given protocol
// type to check against.
//
// This example demonstrates writing data using several fungible types of
// integer containers to a stream and then reading the data out into a
// std::vector. The example also includes a simple header check to validate the
// protocol as a whole.
//

namespace {

// A simple header with magic value and version numbers.
struct Header {
  enum : std::uint32_t { kMagic = 0xdeadbeef };
  enum : std::uint32_t { kVersionMajor = 1, kVersionMinor = 0 };

  // Returns a Header instance with magic and version numbers set.
  static Header Make() { return {kMagic, kVersionMajor, kVersionMinor}; }

  // Returns true if the magic and major version numbers match the constants.
  // The minor version is not checked.
  explicit operator bool() const {
    return magic == kMagic && version_major == kVersionMajor;
  }

  // Data members that comprise the header and define its wire format.
  std::uint32_t magic;
  std::uint32_t version_major;
  std::uint32_t version_minor;

  // Make this header type serializable by describing it to the serialization
  // engine..
  NOP_STRUCTURE(Header, magic, version_major, version_minor);
};

// Protocol type for the header of the message.
using HeaderProto = Protocol<Header>;

// Protocol type for the body of the message.
using BodyProto = Protocol<std::vector<int>>;

// Writes a message with four integers in the message body. In this instance the
// length of the body is always the same, so std::array is used in place of
// std::vector to avoid dynamic allocation. This is allowed by the protocol
// system because std::array<T> is fungible with std::vector<T> for the same
// type T.
template <typename Serializer>
Status<void> WriteMessage(Serializer* serializer, int a, int b, int c, int d) {
  auto status = HeaderProto::Write(serializer, Header::Make());
  if (!status)
    return status;

  return BodyProto::Write(serializer, std::array<int, 4>{{a, b, c, d}});
}

// Writes a message with a compile-time sized std::array of integers in the
// message body. Like the overload above, this instance uses std::array in place
// of std::vector to avoid dynamic allocation.
template <typename Serializer, std::size_t Size>
Status<void> WriteMessage(Serializer* serializer,
                          const std::array<int, Size>& array) {
  auto status = HeaderProto::Write(serializer, Header::Make());
  if (!status)
    return status;

  return BodyProto::Write(serializer, array);
};

// Writes a message with a compile-time sized array of integers in the message
// body. Regular arrays may also be used in place of std::vector.
template <typename Serializer, std::size_t Size>
Status<void> WriteMessage(Serializer* serializer, const int (&array)[Size]) {
  auto status = HeaderProto::Write(serializer, Header::Make());
  if (!status)
    return status;

  return BodyProto::Write(serializer, array);
}

// Writes a message with a std::vector of integers in the message body.
template <typename Serializer>
Status<void> WriteMessage(Serializer* serializer,
                          const std::vector<int>& vector) {
  auto status = HeaderProto::Write(serializer, Header::Make());
  if (!status)
    return status;

  return BodyProto::Write(serializer, vector);
}

// Reads a message by first reading and validating the header and then reading
// the body. Returns the body as a std::vector<int>.
template <typename Deserializer>
Status<std::vector<int>> ReadMessage(Deserializer* deserializer) {
  Header header;
  auto status = HeaderProto::Read(deserializer, &header);
  if (!status)
    return status.error();
  else if (!header)
    return ErrorStatus::ProtocolError;

  std::vector<int> body;
  status = BodyProto::Read(deserializer, &body);
  if (!status)
    return status.error();

  return {std::move(body)};
}

// Prints an error message to std::cerr when the Status<T> || Die() expression
// evaluates to false.
auto Die() { return nop::Die(std::cerr); }

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Construct a serializer to output to a std::stringstream.
  Serializer<StreamWriter<std::stringstream>> serializer;

  // Write a message to the stream using the first overload of WriteMessage.
  WriteMessage(&serializer, 1, 2, 3, 4) || Die();

  // Write a message to the stream using the second overload of WriteMessage.
  WriteMessage(&serializer, std::array<int, 6>{{5, 6, 7, 8, 9, 10}}) || Die();

  // Write a message to the stream using the third overload of WriteMessage.
  WriteMessage(&serializer, {11, 22, 33, 44, 55, 66, 77, 88, 99}) || Die();

  // Write a message to the stream using the fourth overload of WriteMessage.
  WriteMessage(&serializer, std::vector<int>(42, 20)) || Die();

  // Print the serialized buffer in hexadecimal to demonstrate the wire format.
  std::cout << "Serialized data: "
            << StringToHex(serializer.writer().stream().str()) << std::endl;

  // Construct a deserializer to read from a std::stringstream and pass it the
  // serialized data from the serializer.
  Deserializer<StreamReader<std::stringstream>> deserializer{
      std::move(serializer.writer().stream())};

  // Read the messages written by each overload of WriteMessage and print the
  // resulting values.
  for (int i = 0; i < 4; i++) {
    auto return_status = ReadMessage(&deserializer) || Die();
    std::cout << "Read: " << return_status.get() << std::endl;
  }

  return 0;
}
