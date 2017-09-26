#include <iostream>
#include <sstream>

#include <nop/serializer.h>
#include <nop/status.h>
#include <nop/types/variant.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"
#include "string_to_hex.h"

using nop::Deserializer;
using nop::EmptyVariant;
using nop::ErrorStatus;
using nop::Serializer;
using nop::Status;
using nop::StreamReader;
using nop::StreamWriter;
using nop::StringToHex;
using nop::Variant;

//
// Example of using nop::Variant to serialize one of several types. This example
// defines two different user-defined types that are serialized and then
// deserialized. After deserialization the type of the data received may be
// determined at runtime.
//

namespace {

// First user-defined type.
struct MessageA {
  std::uint32_t a;
  std::string b;
  std::vector<short> c;
  NOP_MEMBERS(MessageA, a, b, c);
};

// Second user-defined type.
struct MessageB {
  std::uint64_t x;
  std::vector<int> y;
  std::pair<std::string, std::string> z;
  NOP_MEMBERS(MessageB, x, y, z);
};

// Utility to print MessageA to the debug stream.
std::ostream& operator<<(std::ostream& stream, const MessageA& message) {
  stream << "MessageA{" << message.a << ", " << message.b << ", " << message.c
         << "}";
  return stream;
}

// Utility to print MessageB to the debug strem.
std::ostream& operator<<(std::ostream& stream, const MessageB& message) {
  stream << "MessageB{" << message.x << ", " << message.y << ", " << message.z
         << "}";
  return stream;
}

// Utility to print nop::EmptyVariant to the debug stream.
std::ostream& operator<<(std::ostream& stream, EmptyVariant) {
  stream << "EmptyVariant";
  return stream;
}

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Construct a serializer to output to a std::stringstream.
  Serializer<StreamWriter<std::stringstream>> serializer;

  // Define a variant that can hold MessageA or MessageB values.
  using Messages = Variant<MessageA, MessageB>;

  // Print the sizes of each type.
  std::cout << "sizeof(MessageA): " << sizeof(MessageA) << std::endl;
  std::cout << "sizeof(MessageB): " << sizeof(MessageB) << std::endl;
  std::cout << "sizeof(Messages): " << sizeof(Messages) << std::endl;

  // Serialize a MessageA value.
  auto status = serializer.Write(Messages{MessageA{1, "foo", {1, 2, 3, 4}}});
  CHECK_STATUS(status);

  // Serialize a MessageB value.
  status =
      serializer.Write(Messages{MessageB{1, {1, 2, 3, 4}, {"foo", "bar"}}});
  CHECK_STATUS(status);

  // Get the serialized data and print it to the debug stream.
  std::string data = serializer.writer().stream().str();
  std::cout << "Serialized data: " << StringToHex(data) << std::endl;

  // Construct a deserializer that takes input from a std::stringstring. Use the
  // serialized data to initialize the stream.
  Deserializer<StreamReader<std::stringstream>> deserializer{data};

  // Visit the empty variant to show that it is empty.
  Messages message;
  message.Visit([](const auto& value) { std::cout << value << std::endl; });

  // Deserialize the first message into the variant variable.
  status = deserializer.Read(&message);
  CHECK_STATUS(status);

  // Visit the variant and print out the value to show what type the variant
  // contains.
  message.Visit([](const auto& value) { std::cout << value << std::endl; });

  // Deserialize the second message into the variant variable.
  status = deserializer.Read(&message);
  CHECK_STATUS(status);

  // Visit the variant and print out the value to show what type the variant
  // contains now.
  message.Visit([](const auto& value) { std::cout << value << std::endl; });

  return 0;
}
