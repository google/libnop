#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nop/serializer.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

using nop::Deserializer;
using nop::Serializer;
using nop::StreamReader;
using nop::StreamWriter;

// Here we describe struct tm from the standard library, which is code we don't
// own and can't change to include an annotation. The NOP_STRUCTURE() macro
// annotates types we don't own so that the serializer can understand how to
// work with them. The annotation must be in the same namespace as the structure
// is originally defined in.
// NOTE: struct tm is orignally defined in the global namespace, even though the
// header <ctime> also aliases it into the std namespace. The annotation must be
// defined in the ORIGINAL namespace of the annotated type because the compiler
// only considers the original namespace during name lookup.
NOP_STRUCTURE(tm, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday,
              tm_yday, tm_isdst);

namespace {

// A user-defined type with private memebrs may be annotated. Annotated types
// must be default constrictible. Annotation does not affect the size or
// function of the class: all of the necessary information is captured in the
// type system.
class TypeB {
 public:
  TypeB() = default;
  TypeB(const std::string& a, std::vector<int> b) : a_{a}, b_{std::move(b)} {}

  TypeB(const TypeB&) = default;
  TypeB(TypeB&&) = default;
  TypeB& operator=(const TypeB&) = default;
  TypeB& operator=(TypeB&&) = default;

  const std::string a() const { return a_; }
  const std::vector<int> b() const { return b_; }

 private:
  std::string a_;
  std::vector<int> b_;

  NOP_MEMBERS(TypeB, a_, b_);
};

// All enum and enum class types are serializable.
enum class EnumA {
  Foo,
  Bar,
  Baz,
};

// A structure with internal annotation. The annotation only defines a nested
// type, which does not affect the size or funcion of the structure.
struct MessageA {
  int a;
  float b;
  std::string c;
  std::vector<TypeB> d;
  std::tm e;
  EnumA f;

  NOP_MEMBERS(MessageA, a, b, c, d, e, f);
};

// Printing utilities. These are only used to display data in this example and
// are not required for libnop operation.

// Prints a std::vector<T> to the given stream. This template will work for any
// type T that has an operator<< overload.
template <typename T, typename Allocator>
std::ostream& operator<<(std::ostream& stream,
                         const std::vector<T, Allocator>& vector) {
  stream << "vector{";

  const std::size_t length = vector.size();
  std::size_t count = 0;

  for (const auto& element : vector) {
    stream << element;
    if (count < length - 1)
      stream << ", ";
    count++;
  }

  stream << "}";
  return stream;
}

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

// Prints a MessageA type to the given stream.
std::ostream& operator<<(std::ostream& stream, const MessageA& message) {
  stream << "MessageA{" << message.a << ", " << message.b << ", " << message.c
         << ", " << message.d << ", " << message.e << ", " << message.f << "}";
  return stream;
}

// Prints a TypeB type to the given stream.
std::ostream& operator<<(std::ostream& stream, const TypeB& value) {
  stream << "TypeB{" << value.a() << ", " << value.b() << "}";
  return stream;
}

// Prints the bytes of a string in hexadecimal.
std::string StringToHex(const std::string& input) {
  static const char* const lut = "0123456789ABCDEF";
  const size_t len = input.length();

  std::string output;
  output.reserve(2 * len);
  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = input[i];
    output.push_back(lut[c >> 4]);
    output.push_back(lut[c & 15]);
    if (i < len - 1)
      output.push_back(' ');
  }

  return output;
}

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Construct a serializer to output to a std::stringstream.
  Serializer<StreamWriter<std::stringstream>> serializer;

  // Get the local time to write out with the message structure.
  std::time_t time = std::time(nullptr);
  std::tm local_time = *std::localtime(&time);

  // Write a MessageA structure to the stream.
  MessageA message_out{10,         20.f,
                       "foo",      {{"bar", {1, 2, 3}}, {"baz", {4, 5, 6}}},
                       local_time, EnumA::Baz};
  std::cout << "Writing: " << message_out << std::endl << std::endl;

  auto status = serializer.Write(message_out);
  if (!status) {
    std::cerr << "Serialization failed: " << status.GetErrorMessage()
              << std::endl;
    return -1;
  }

  // Dump the serialized data as a hex string.
  std::string data = serializer.writer().stream().str();
  std::cout << "Serialized data: " << StringToHex(data) << std::endl
            << std::endl;

  // Construct a deserializer to read from a std::stringstream.
  Deserializer<StreamReader<std::stringstream>> deserializer{data};

  // Read a MessageA structure from the stream.
  MessageA message_in;
  status = deserializer.Read(&message_in);
  if (!status) {
    std::cerr << "Deserialization failed: " << status.GetErrorMessage()
              << std::endl;
    return -1;
  }

  // Print the MessageA structure.
  std::cout << "Read   : " << message_in << std::endl;

  return 0;
}
