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

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/utility/die.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"
#include "string_to_hex.h"

using nop::Deserializer;
using nop::Optional;
using nop::Serializer;
using nop::StreamReader;
using nop::StreamWriter;
using nop::StringToHex;

//
// An example of reading and writing structured data to streams. This example
// defines a couple of serializable types and demonstrates the serialization of
// an externally defined structure from the C standard library.
//

// Here we describe struct tm from the standard library, which is code we don't
// own and can't change to include an annotation. The NOP_EXTERNAL_STRUCTURE()
// macro annotates types we don't own so that the serializer can understand how
// to work with them. The annotation must be in the same namespace as the
// structure is originally defined in. NOTE: struct tm is orignally defined in
// the global namespace, even though the header <ctime> also aliases it into the
// std namespace. The annotation must be defined in the ORIGINAL namespace of
// the annotated type because the compiler only considers the original namespace
// during name lookup.
NOP_EXTERNAL_STRUCTURE(tm, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year,
                       tm_wday, tm_yday, tm_isdst);

namespace {

// A user-defined type with private memebrs may be annotated. Annotated types
// must be default constrictible. Annotation does not affect the size or
// function of the class: all of the necessary information is captured in the
// type system.
class UserDefinedA {
 public:
  UserDefinedA() = default;
  UserDefinedA(const std::string& a, std::vector<int> b)
      : a_{a}, b_{std::move(b)} {}

  UserDefinedA(const UserDefinedA&) = default;
  UserDefinedA(UserDefinedA&&) = default;
  UserDefinedA& operator=(const UserDefinedA&) = default;
  UserDefinedA& operator=(UserDefinedA&&) = default;

  const std::string a() const { return a_; }
  const std::vector<int> b() const { return b_; }

 private:
  std::string a_;
  std::vector<int> b_;

  NOP_STRUCTURE(UserDefinedA, a_, b_);
};

// All enum and enum class types are serializable.
enum class EnumA {
  Foo,
  Bar,
  Baz,
};

// A structure with internal annotation. The annotation only defines a nested
// type, which does not affect the size or funcion of the structure.
struct UserDefinedB {
  int a;
  float b;
  std::string c;
  std::vector<UserDefinedA> d;
  std::tm e;
  EnumA f;
  Optional<std::string> g;

  NOP_STRUCTURE(UserDefinedB, a, b, c, d, e, f, g);
};

// Prints a struct tm to the given stream.
std::ostream& operator<<(std::ostream& stream, const std::tm& tm) {
  stream << "tm{" << tm.tm_sec << ", " << tm.tm_min << ", " << tm.tm_hour
         << ", " << tm.tm_mday << ", " << tm.tm_mon << ", " << tm.tm_year
         << ", " << tm.tm_wday << ", " << tm.tm_yday << ", " << tm.tm_isdst
         << "}";
  return stream;
}

// Prints a EnumA value to the given stream.
std::ostream& operator<<(std::ostream& stream, const EnumA& value) {
  switch (value) {
    case EnumA::Foo:
      stream << "EnumA::Foo";
      break;
    case EnumA::Bar:
      stream << "EnumA::Bar";
      break;
    case EnumA::Baz:
      stream << "EnumA::Baz";
      break;
    default:
      using Type = std::underlying_type<EnumA>::type;
      stream << "EnumA::<" + std::to_string(static_cast<Type>(value)) + ">";
      break;
  }
  return stream;
}

// Prints an Optional<T> type to the given stream.
template <typename T>
std::ostream& operator<<(std::ostream& stream, const Optional<T>& value) {
  stream << "Optional{";
  if (value)
    stream << value.get();
  else
    stream << "<empty>";
  stream << "}";
  return stream;
}

// Prints a UserDefinedB type to the given stream.
std::ostream& operator<<(std::ostream& stream, const UserDefinedB& message) {
  stream << "UserDefinedB{" << message.a << ", " << message.b << ", "
         << message.c << ", " << message.d << ", " << message.e << ", "
         << message.f << ", " << message.g << "}";
  return stream;
}

// Prints a UserDefinedA type to the given stream.
std::ostream& operator<<(std::ostream& stream, const UserDefinedA& message) {
  stream << "UserDefinedA{" << message.a() << ", " << message.b() << "}";
  return stream;
}

// Prints an error message to std::cerr when the Status<T> || Die() expression
// evaluates to false.
auto Die() { return nop::Die(std::cerr); }

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Construct a serializer to output to a std::stringstream.
  Serializer<StreamWriter<std::stringstream>> serializer;

  // Get the local time to write out with the message structure.
  std::time_t time = std::time(nullptr);
  std::tm local_time = *std::localtime(&time);

  // Write a UserDefinedB structure to the stream.
  UserDefinedB message_out{10,
                           20.f,
                           "foo",
                           {{"bar", {1, 2, 3}}, {"baz", {4, 5, 6}}},
                           local_time,
                           EnumA::Baz,
                           Optional<std::string>{"bif"}};
  std::cout << "Writing: " << message_out << std::endl << std::endl;

  serializer.Write(message_out) || Die();

  // Dump the serialized data as a hex string.
  std::string data = serializer.writer().stream().str();
  std::cout << "Serialized data: " << StringToHex(data) << std::endl
            << std::endl;

  // Construct a deserializer to read from a std::stringstream.
  Deserializer<StreamReader<std::stringstream>> deserializer{data};

  // Read a UserDefinedB structure from the stream.
  UserDefinedB message_in;
  deserializer.Read(&message_in) || Die();

  // Print the UserDefinedB structure.
  std::cout << "Read   : " << message_in << std::endl;
  return 0;
}
