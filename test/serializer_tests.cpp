#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <nop/base/array.h>
#include <nop/base/enum.h>
#include <nop/base/map.h>
#include <nop/base/members.h>
#include <nop/base/pair.h>
#include <nop/base/serializer.h>
#include <nop/base/string.h>
#include <nop/base/tuple.h>
#include <nop/base/utility.h>
#include <nop/base/vector.h>

#include "test_reader.h"
#include "test_writer.h"

using nop::Deserializer;
using nop::EnableIfIntegral;
using nop::Encoding;
using nop::EncodingByte;
using nop::Serializer;
using nop::Status;
using nop::TestReader;
using nop::TestWriter;

namespace {

template <typename I, typename Enabled = EnableIfIntegral<I>>
std::vector<std::uint8_t> Integer(I integer) {
  std::vector<uint8_t> vector(sizeof(I));
  const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&integer);
  for (std::size_t i = 0; i < sizeof(I); i++)
    vector[i] = bytes[i];
  return vector;
}

std::vector<std::uint8_t> Item(const std::vector<std::uint8_t>& vector) {
  return vector;
}

// Only define Item for one-byte integral type. Larger integral literals must
// use Integer() explicitly.
auto Item(std::uint8_t value) { return Integer(value); }

std::vector<std::uint8_t> Item(EncodingByte prefix) {
  return {static_cast<std::uint8_t>(prefix)};
}

auto Item(const std::string& string) {
  return std::vector<std::uint8_t>(string.begin(), string.end());
}

void Append(std::vector<std::uint8_t>* /*vector*/) {}

template <typename First, typename... Rest>
void Append(std::vector<std::uint8_t>* vector, First&& first, Rest&&... rest) {
  auto value = Item(std::forward<First>(first));
  vector->insert(vector->end(), value.begin(), value.end());
  Append(vector, std::forward<Rest>(rest)...);
}

template <typename... Args>
std::vector<std::uint8_t> Compose(Args&&... args) {
  std::vector<uint8_t> vector;
  Append(&vector, std::forward<Args>(args)...);
  return vector;
}

#if 0
template <typename MapType, typename Writer>
void InsertKeyValue(writer* writer, std::size_t size) {
  MapType map;
  for (std::size_t i = 0; i < size; i++) {
    map.emplace(i, i);
  }
  for (const auto& element : map) {

    Serialize(element.first, writer);
    Serialize(element.second, writer);
  }
}
#endif

struct TestA {
  int a;
  std::string b;

  bool operator==(const TestA& other) const {
    return a == other.a && b == other.b;
  }

 private:
  NOP_MEMBERS(TestA, a, b);
};

enum class EnumA : std::uint8_t {
  A = 1,
  B = 127,
  C = 128,
  D = 255,
};

struct TestB {
  TestA a;
  EnumA b;

  bool operator==(const TestB& other) const {
    return a == other.a && b == other.b;
  }

 private:
  NOP_MEMBERS(TestB, a, b);
};

}  // anonymous namespace

#if 0
// This test verifies that the compiler outputs a custom error message when an
// unsupported type is pass to the serializer.
TEST(Serializer, None) {
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};

  struct None {};
  auto status = serializer.Write(None{});
  ASSERT_TRUE(status.ok());
}
#endif

TEST(Serializer, bool) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};

  EXPECT_EQ(1U, serializer.GetSize(true));
  EXPECT_EQ(1U, serializer.GetSize(false));

  auto status = serializer.Write(true);
  ASSERT_TRUE(status.ok());

  expected = Compose(true);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  status = serializer.Write(false);
  ASSERT_TRUE(status.ok());

  expected = Compose(false);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, bool) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  bool value;

  reader.Set(Compose(EncodingByte::True));
  auto status = deserializer.Read(&value);
  ASSERT_TRUE(status.ok());
  EXPECT_EQ(true, value);

  reader.Set(Compose(EncodingByte::False));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status.ok());
  EXPECT_EQ(false, value);
}

TEST(Serializer, Vector) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::vector<std::uint8_t> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, 1, 2, 3, 4);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<int> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<std::int64_t> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, Integer<std::int64_t>(1),
                       Integer<std::int64_t>(2), Integer<std::int64_t>(3),
                       Integer<std::int64_t>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<std::string> value = {"abc", "def", "123", "456"};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Vector) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    reader.Set(Compose(EncodingByte::Binary, 4, 1, 2, 3, 4));

    std::vector<std::uint8_t> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::vector<std::uint8_t> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4, Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4)));

    std::vector<int> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4, Integer<std::uint64_t>(1),
                       Integer<std::uint64_t>(2), Integer<std::uint64_t>(3),
                       Integer<std::uint64_t>(4)));

    std::vector<std::uint64_t> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::vector<std::uint64_t> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456"));

    std::vector<std::string> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::vector<std::string> expected = {"abc", "def", "123", "456"};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Array) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, 1, 2, 3, 4);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<int, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<std::int64_t, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Binary, 4, Integer<std::int64_t>(1),
                       Integer<std::int64_t>(2), Integer<std::int64_t>(3),
                       Integer<std::int64_t>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<std::string, 4> value = {{"abc", "def", "123", "456"}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Array) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    reader.Set(Compose(EncodingByte::Binary, 4, 1, 2, 3, 4));

    std::array<std::uint8_t, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::array<std::uint8_t, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4, Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4)));

    std::array<int, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::array<int, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4, Integer<std::uint64_t>(1),
                       Integer<std::uint64_t>(2), Integer<std::uint64_t>(3),
                       Integer<std::uint64_t>(4)));

    std::array<std::uint64_t, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::array<std::uint64_t, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456"));

    std::array<std::string, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::array<std::string, 4> expected = {{"abc", "def", "123", "456"}};
    EXPECT_EQ(expected, value);
  }
}
TEST(Serializer, uint8_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;
  uint8_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status.ok());
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status.ok());
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status.ok());
  expected = Compose(EncodingByte::U8, (1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status.ok());
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Serializer, String) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::string value = "abcdefg";

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::String, 7, "abcdefg");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, String) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  std::string expected;
  std::string value;
  Status<void> status;

  reader.Set(Compose(EncodingByte::String, 7, "abcdefg"));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status.ok());

  expected = "abcdefg";
  EXPECT_EQ(expected, value);
}

TEST(Serializer, VectorString) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    const std::string a = "abcdefg";
    const std::string b = "1234567";
    std::vector<std::string> value = {a, b};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Array, 2, EncodingByte::String, 7,
                       "abcdefg", EncodingByte::String, 7, "1234567");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Serializer, Tuple) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::tuple<int, std::string> value(10, "foo");

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected =
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Tuple) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    std::tuple<int, std::string> value;

    reader.Set(
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::tuple<int, std::string> expected(10, "foo");
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Pair) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::pair<int, std::string> value(10, "foo");

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected =
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Pair) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    std::pair<int, std::string> value;

    reader.Set(
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::pair<int, std::string> expected(10, "foo");
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Map) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::map<int, std::string> value = {{{0, "abc"}, {1, "123"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Map) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    std::map<int, std::string> value;

    reader.Set(Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::map<int, std::string> expected = {{{0, "abc"}, {1, "123"}}};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, UnorderedMap) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    std::unordered_map<int, std::string> value = {{{0, "abc"}, {1, "123"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::Map, 2);

    for (const auto& element : value) {
      Append(&expected, element.first, EncodingByte::String,
             element.second.size(), element.second);
    }

    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, UnorderedMap) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    std::unordered_map<int, std::string> value;

    reader.Set(Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    std::unordered_map<int, std::string> expected = {{{0, "abc"}, {1, "123"}}};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Enum) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    EnumA value = EnumA::A;

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(1);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::B;

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(127);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::C;

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::U8, 128);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::D;

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected = Compose(EncodingByte::U8, 255);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Enum) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    EnumA value;

    reader.Set(Compose(1));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    EnumA expected = EnumA::A;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(127));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    EnumA expected = EnumA::B;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(EncodingByte::U8, 128));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    EnumA expected = EnumA::C;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(EncodingByte::U8, 255));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    EnumA expected = EnumA::D;
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Members) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};
  Status<void> status;

  {
    TestA value{10, "foo"};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected =
        Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestB value{{10, "foo"}, EnumA::C};

    status = serializer.Write(value);
    ASSERT_TRUE(status.ok());

    expected =
        Compose(EncodingByte::Structure, 2, EncodingByte::Structure, 2, 10,
                EncodingByte::String, 3, "foo", EncodingByte::U8, 128);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Members) {
  TestReader reader;
  Deserializer<TestReader> deserializer{&reader};
  Status<void> status;

  {
    TestA value;

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3,
                       "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    TestA expected{10, "foo"};
    EXPECT_EQ(expected, value);
  }

  {
    TestB value;

    reader.Set(Compose(EncodingByte::Structure, 2, EncodingByte::Structure, 2,
                       10, EncodingByte::String, 3, "foo", EncodingByte::U8,
                       128));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status.ok());

    TestB expected{{10, "foo"}, EnumA::C};
    EXPECT_EQ(expected, value);
  }
}
