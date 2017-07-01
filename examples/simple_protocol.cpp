#include <array>
#include <iostream>
#include <sstream>
#include <vector>

#include <nop/protocol.h>
#include <nop/serializer.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"

using nop::Deserializer;
using nop::Protocol;
using nop::Serializer;
using nop::StreamReader;
using nop::StreamWriter;

//
// simple_protocol.cpp - Example of using a compile-time protocol check and
// fungible types.
//
// A protocol is a type that defines the valid data format for communication.
// The protocol system supports the notion of fungible types: types that are
// fungible produce the same wire format and can be used interchangeably when
// calling the serialization engine. The class nop::Protocol provides a simple
// way to verify that code conforms to a type-safe protocol by checking that the
// types passed to its Read/Write methods are fungible with the given protocol
// type to check against.
//
// This example demonstrates writing a std::array to a stream and then reading
// it out into a std::vector. The types std::array<T, Size> and std::vector<U,
// Allocator> are fungible when type T and U are fungible. Here T=int and U=int;
// the same type is fungible with itself.

namespace {

// Define a protocol type. This type determines what types are valid to
// read/write.
using MyProtocolType = std::array<int, 4>;

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Construct a serializer to output to a std::stringstream.
  Serializer<StreamWriter<std::stringstream>> serializer;

  std::array<int, 4> array_a{{1, 2, 3, 4}};

  // Serialize the array. nop::Protocol allows this line to compile because the
  // object type is the same as the contract type.
  auto status = Protocol<MyProtocolType>::Write(&serializer, array_a);
  if (!status) {
    std::cerr << "Failed to serialize data: " << status.GetErrorMessage()
              << std::endl;
    return -status.error();
  }
  std::cout << "Wrote: " << array_a << std::endl;

  // Construct a deserializer to read from a std::stringstream and pass it the
  // serialized data from the serializer.
  Deserializer<StreamReader<std::stringstream>> deserializer{
      serializer.writer().stream().str()};

  std::vector<int> vector_a;

  // Deserialize to a vector. nop::Protocol allows this line to compile because
  // std::vector<int> is "fungible" with std::array<int, Size>.
  status = Protocol<MyProtocolType>::Read(&deserializer, &vector_a);
  if (!status) {
    std::cerr << "Failed to deserialize data: " << status.GetErrorMessage()
              << std::endl;
    return -status.error();
  }
  std::cout << "Read : " << vector_a << std::endl;

  return 0;
}
