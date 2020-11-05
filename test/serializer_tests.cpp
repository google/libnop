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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include <nop/base/utility.h>
#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/table.h>
#include <nop/value.h>

#include "mock_reader.h"
#include "mock_writer.h"
#include "test_reader.h"
#include "test_utilities.h"
#include "test_writer.h"

using nop::Append;
using nop::Compose;
using nop::DefaultHandlePolicy;
using nop::DeletedEntry;
using nop::Deserializer;
using nop::EnableIfIntegral;
using nop::Encoding;
using nop::EncodingByte;
using nop::Entry;
using nop::ErrorStatus;
using nop::Float;
using nop::Handle;
using nop::Integer;
using nop::Serializer;
using nop::Status;
using nop::TestReader;
using nop::TestWriter;
using nop::Variant;

using nop::testing::MockReader;
using nop::testing::MockWriter;

using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;

namespace {

struct TestA {
  int a;
  std::string b;

  bool operator==(const TestA& other) const {
    return a == other.a && b == other.b;
  }

 private:
  NOP_STRUCTURE(TestA, a, b);
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
  NOP_STRUCTURE(TestB, a, b);
};

struct TestC {
  NOP_STRUCTURE(TestC);
};

struct TestD {
  int a;
  EnumA b;
  std::string c;

  bool operator==(const TestD& other) const {
    return a == other.a && b == other.b && c == other.c;
  }
};
NOP_EXTERNAL_STRUCTURE(TestD, a, b, c);

template <typename T>
struct TestE {
  T a;
  std::vector<T> b;

  bool operator==(const TestE& other) const {
    return a == other.a && b == other.b;
  }
};
NOP_EXTERNAL_STRUCTURE(TestE, a, b);

template <typename T, typename U>
struct TestF {
  T a;
  U b;

  bool operator==(const TestF& other) const {
    return a == other.a && b == other.b;
  }
};
NOP_EXTERNAL_STRUCTURE(TestF, a, b);

struct TestG {
  int a;
  TestF<int, std::string> b;

  bool operator==(const TestG& other) const {
    return a == other.a && b == other.b;
  }

 private:
  NOP_STRUCTURE(TestG, a, b);
};

struct TestH {
  char data[128];
  unsigned char size;
};
NOP_EXTERNAL_STRUCTURE(TestH, (data, size));

struct TestH2 {
  std::array<char, 128> data;
  unsigned char size;
};
NOP_EXTERNAL_STRUCTURE(TestH2, (data, size));

struct TestI {
  std::string names[5];
  std::size_t size;

  bool operator==(const TestI& other) const {
    if (size != other.size)
      return false;
    for (std::size_t i = 0; i < size; i++) {
      if (names[i] != other.names[i])
        return false;
    }
    return true;
  }

  NOP_STRUCTURE(TestI, (names, size));
};

// A special rule allows data to be read/written past the end of the structure
// when a logical buffer pair has an array part of size 1 at the end of the
// structure. This permits serialization of the common dynamically sized buffer
// pattern in C code.
template <typename T>
struct TestJ {
  size_t size;
  T data[1];
};
NOP_EXTERNAL_STRUCTURE(TestJ, (data, size));
NOP_EXTERNAL_UNBOUNDED_BUFFER(TestJ);

template <typename T>
std::unique_ptr<TestJ<T>, decltype(&std::free)> MakeTestJ(
    std::size_t capacity) {
  return {static_cast<TestJ<T>*>(
              std::calloc(1, offsetof(TestJ<T>, data) + capacity)),
          &std::free};
}

template <typename T, std::size_t Size>
auto MakeTestJ(const T (&data)[Size]) {
  auto value = MakeTestJ<T>(Size);
  std::copy(&data[0], &data[Size], &value->data[0]);
  value->size = Size;
  return value;
}

struct TableA1 {
  TableA1() = default;
  TableA1(std::string name) : name{std::move(name)} {}
  TableA1(std::vector<std::string> attributes)
      : attributes{std::move(attributes)} {}
  TableA1(std::string name, std::vector<std::string> attributes)
      : name{std::move(name)}, attributes{std::move(attributes)} {}

  bool operator==(const TableA1& other) const {
    return name == other.name && attributes == other.attributes;
  }

  Entry<std::string, 0> name;
  Entry<std::vector<std::string>, 1> attributes;

  NOP_TABLE_HASH(15, TableA1, name, attributes);
};

struct TableA2 {
  TableA2() = default;
  TableA2(std::string name) : name{std::move(name)} {}
  TableA2(std::string name, std::string address)
      : name{std::move(name)}, address{std::move(address)} {}

  bool operator==(const TableA2& other) const {
    return name == other.name && address == other.address;
  }

  Entry<std::string, 0> name;
  Entry<std::vector<std::string>, 1, DeletedEntry> attributes;
  Entry<std::string, 2> address;

  NOP_TABLE_HASH(15, TableA2, name, attributes, address);
};

template <typename T>
struct ValueWrapper {
  T value;
  NOP_VALUE(ValueWrapper, value);
};

template <typename T, std::size_t Length>
struct ArrayWrapper {
  std::array<T, Length> data{};
  std::size_t size{0};
  NOP_VALUE(ArrayWrapper, (data, size));
};

template <typename T>
struct CDynamicArray {
  std::size_t size;
  T data[1];

  NOP_VALUE(CDynamicArray, (data, size));
  NOP_UNBOUNDED_BUFFER(CDynamicArray);
};

template <typename T>
std::unique_ptr<CDynamicArray<T>, decltype(&std::free)> MakeCDynamicArray(
    std::size_t capacity) {
  return {static_cast<CDynamicArray<T>*>(
              std::calloc(1, offsetof(CDynamicArray<T>, data) + capacity)),
          &std::free};
}

template <typename T, std::size_t Size>
auto MakeCDynamicArray(const T (&data)[Size]) {
  auto value = MakeCDynamicArray<T>(Size);
  std::copy(&data[0], &data[Size], &value->data[0]);
  value->size = Size;
  return value;
}

}  // anonymous namespace

#if 0
// This test verifies that the compiler outputs a custom error message when an
// unsupported type is pass to the serializer.
TEST(Serializer, None) {
  TestWriter writer;
  Serializer<TestWriter> serializer{&writer};

  struct None {};
  auto status = serializer.Write(None{});
  ASSERT_TRUE(status);
}
#endif

TEST(Deserializer, FailOnMatchPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(1)
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  bool value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Deserializer, FailOnReadArithmetic) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(1)
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::U8)),
                Return(Status<void>{})));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::uint8_t value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, bool) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  EXPECT_EQ(1U, serializer.GetSize(true));
  EXPECT_EQ(1U, serializer.GetSize(false));

  auto status = serializer.Write(true);
  ASSERT_TRUE(status);

  expected = Compose(true);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  status = serializer.Write(false);
  ASSERT_TRUE(status);

  expected = Compose(false);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, bool) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  bool value;

  reader.Set(Compose(EncodingByte::True));
  auto status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(true, value);

  reader.Set(Compose(EncodingByte::False));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(false, value);
}

TEST(Serializer, IntegerVectorFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::uint8_t> value = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerVectorFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::string> value = {"a", "b", "c", "d"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerVectorFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::uint8_t> value = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerVectorFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::string> value = {"a", "b", "c", "d"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerVectorFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::uint8_t> value = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerVectorFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::string> value = {"a", "b", "c", "d"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerVectorFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding length fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::uint8_t> value = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerVectorFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding length fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::vector<std::string> value = {"a", "b", "c", "d"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, IntegerVectorFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::uint8_t> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerVectorFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerVectorFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::uint8_t> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerVectorFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::uint16_t> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, IntegerVectorFailOnEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  std::size_t length_bytes = 2 * sizeof(std::uint16_t);

  EXPECT_CALL(reader, Ensure(length_bytes))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(length_bytes), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::uint16_t> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerVectorFailOnReadElements) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::uint8_t> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerVectorFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::vector<std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Vector) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::vector<std::uint8_t> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4, 1, 2, 3, 4);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<int> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<std::int64_t> value = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(std::int64_t),
                       Integer<std::int64_t>(1), Integer<std::int64_t>(2),
                       Integer<std::int64_t>(3), Integer<std::int64_t>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::vector<std::string> value = {"abc", "def", "123", "456"};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Vector) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    reader.Set(Compose(EncodingByte::Binary, 4, 1, 2, 3, 4));

    std::vector<std::uint8_t> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::vector<std::uint8_t> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4)));

    std::vector<int> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(std::uint64_t),
                       Integer<std::uint64_t>(1), Integer<std::uint64_t>(2),
                       Integer<std::uint64_t>(3), Integer<std::uint64_t>(4)));

    std::vector<std::uint64_t> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::vector<std::uint64_t> expected = {1, 2, 3, 4};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456"));

    std::vector<std::string> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::vector<std::string> expected = {"abc", "def", "123", "456"};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, IntegerStdArrayFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerCArrayFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::uint8_t value[4] = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerStdArrayFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::string, 4> value = {{"1", "2", "3", "4"}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerCArrayFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() immediately indicates no writes possible.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value[4] = {"1", "2", "3", "4"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerStdArrayFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerCArrayFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::uint8_t value[4] = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerStdArrayFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::string, 4> value = {{"1", "2", "3", "4"}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerCArrayFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value[4] = {"1", "2", "3", "4"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerStdArrayFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerCArrayFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::uint8_t value[4] = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerStdArrayFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::string, 4> value = {{"1", "2", "3", "4"}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerCArrayFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value[4] = {"1", "2", "3", "4"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerStdArrayFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding length fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, IntegerCArrayFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding payload fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::uint8_t value[4] = {1, 2, 3, 4};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerStdArrayFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding length fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::array<std::string, 4> value = {{"1", "2", "3", "4"}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, NonIntegerCArrayFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding payload fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value[4] = {"1", "2", "3", "4"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, IntegerStdArrayFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::uint8_t, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerCArrayFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::uint8_t value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerStdArrayFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::string, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerCArrayFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerStdArrayFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::uint8_t, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerCArrayFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::uint8_t value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerStdArrayFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::string, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerCArrayFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerStdArrayFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::uint8_t, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, IntegerCArrayFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::uint8_t value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, NonIntegerStdArrayFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::string, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, NonIntegerCArrayFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, IntegerStdArrayFailOnReadElements) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::uint8_t, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, IntegerCArrayFailOnReadElements) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::uint8_t value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerStdArrayFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::array<std::string, 4> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, NonIntegerCArrayFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value[4];
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Array) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::array<std::uint8_t, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4, 1, 2, 3, 4);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::uint8_t value[] = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4, 1, 2, 3, 4);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<int, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    int value[] = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<std::int64_t, 4> value = {{1, 2, 3, 4}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(std::int64_t),
                       Integer<std::int64_t>(1), Integer<std::int64_t>(2),
                       Integer<std::int64_t>(3), Integer<std::int64_t>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::int64_t value[] = {1, 2, 3, 4};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Binary, 4 * sizeof(std::int64_t),
                       Integer<std::int64_t>(1), Integer<std::int64_t>(2),
                       Integer<std::int64_t>(3), Integer<std::int64_t>(4));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<std::string, 4> value = {{"abc", "def", "123", "456"}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::string value[] = {"abc", "def", "123", "456"};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Array) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    reader.Set(Compose(EncodingByte::Binary, 4, 1, 2, 3, 4));

    std::array<std::uint8_t, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::array<std::uint8_t, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4, 1, 2, 3, 4));

    std::uint8_t value[4];
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    const std::uint8_t expected[] = {1, 2, 3, 4};
    EXPECT_TRUE(std::equal(std::begin(value), std::end(value),
                           std::begin(expected), std::end(expected)));
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4)));

    std::array<int, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::array<int, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(int), Integer<int>(1),
                       Integer<int>(2), Integer<int>(3), Integer<int>(4)));

    int value[4];
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    const int expected[] = {1, 2, 3, 4};
    EXPECT_TRUE(std::equal(std::begin(value), std::end(value),
                           std::begin(expected), std::end(expected)));
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(std::uint64_t),
                       Integer<std::uint64_t>(1), Integer<std::uint64_t>(2),
                       Integer<std::uint64_t>(3), Integer<std::uint64_t>(4)));

    std::array<std::uint64_t, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::array<std::uint64_t, 4> expected = {{1, 2, 3, 4}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Binary, 4 * sizeof(std::uint64_t),
                       Integer<std::uint64_t>(1), Integer<std::uint64_t>(2),
                       Integer<std::uint64_t>(3), Integer<std::uint64_t>(4)));

    std::uint64_t value[4];
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    const std::uint64_t expected[] = {1, 2, 3, 4};
    EXPECT_TRUE(std::equal(std::begin(value), std::end(value),
                           std::begin(expected), std::end(expected)));
  }

  {
    reader.Set(Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456"));

    std::array<std::string, 4> value;
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::array<std::string, 4> expected = {{"abc", "def", "123", "456"}};
    EXPECT_EQ(expected, value);
  }

  {
    reader.Set(Compose(EncodingByte::Array, 4, EncodingByte::String, 3, "abc",
                       EncodingByte::String, 3, "def", EncodingByte::String, 3,
                       "123", EncodingByte::String, 3, "456"));

    std::string value[4];
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    const std::string expected[] = {"abc", "def", "123", "456"};
    EXPECT_TRUE(std::equal(std::begin(value), std::end(value),
                           std::begin(expected), std::end(expected)));
  }
}

TEST(Serializer, char) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  char value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = static_cast<char>(1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, (1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, char) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  char value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(static_cast<char>(0), value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(static_cast<char>(127), value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(static_cast<char>(0), value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ('\xff', value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, uint8_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::uint8_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, (1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, uint8_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::uint8_t value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127U, value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffU, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, uint16_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::uint16_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, Integer<std::uint8_t>(1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U16.
  value = (1 << 8);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(1 << 8));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U16.
  value = 0xffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, uint16_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::uint16_t value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127U, value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffU, value);

  // Min U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffU, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, uint32_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::uint32_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, Integer<std::uint8_t>(1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U16.
  value = (1 << 8);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(1 << 8));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U16.
  value = 0xffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U32.
  value = (1 << 16);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(1 << 16));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U32.
  value = 0xffffffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, uint32_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::uint32_t value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127U, value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffU, value);

  // Min U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffU, value);

  // Min U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffffffU, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, uint64_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::uint64_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, Integer<std::uint8_t>(1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U16.
  value = (1 << 8);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(1 << 8));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U16.
  value = 0xffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U32.
  value = (1 << 16);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(1 << 16));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U32.
  value = 0xffffffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U64.
  value = (1LLU << 32);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U64, Integer<std::uint64_t>(1LLU << 32));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U64.
  value = 0xffffffffffffffffLLU;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::U64, Integer<std::uint64_t>(0xffffffffffffffffLLU));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, uint64_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::uint64_t value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127U, value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffU, value);

  // Min U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffU, value);

  // Min U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffffffU, value);

  // Min U64.
  reader.Set(Compose(EncodingByte::U64, Integer<std::uint64_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U64.
  reader.Set(Compose(EncodingByte::U64,
                     Integer<std::uint64_t>(0xffffffffffffffffLLU)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffffffffffffffLLU, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, int8_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::int8_t value;

  // Min NEGATIVE FIXINT.
  value = -64;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max NEGATIVE FIXINT.
  value = -1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = 127;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I8.
  value = -128;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-128));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I8.
  value = -65;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-65));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, int8_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::int8_t value;

  // Min NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-64, value);

  // Max NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-1, value);

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(-128)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-128, value);

  // Max I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(127)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, int16_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::int16_t value;

  // Min NEGATIVE FIXINT.
  value = -64;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max NEGATIVE FIXINT.
  value = -1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = 127;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I8.
  value = -128;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-128));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I8.
  value = -65;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-65));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I16.
  value = -32768;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(-32768));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I16.
  value = 32767;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(32767));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, int16_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::int16_t value;

  // Min NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-64, value);

  // Max NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-1, value);

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(-128)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-128, value);

  // Max I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(127)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(-32768)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-32768, value);

  // Max I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(32767)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(32767, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, int32_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::int32_t value;

  // Min NEGATIVE FIXINT.
  value = -64;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max NEGATIVE FIXINT.
  value = -1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = 127;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I8.
  value = -128;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-128));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I8.
  value = -65;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-65));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I16.
  value = -32768;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(-32768));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I16.
  value = 32767;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(32767));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I32.
  value = -2147483648;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I32, Integer<std::int32_t>(-2147483648));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I32.
  value = 2147483647;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I32, Integer<std::int32_t>(2147483647));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, int32_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::int32_t value;

  // Min NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-64, value);

  // Max NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-1, value);

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(-128)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-128, value);

  // Max I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(127)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(-32768)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-32768, value);

  // Max I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(32767)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(32767, value);

  // Min I32.
  reader.Set(Compose(EncodingByte::I32, Integer<std::int32_t>(-2147483648)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-2147483648, value);

  // Max I32.
  reader.Set(Compose(EncodingByte::I32, Integer<std::int32_t>(2147483647)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(2147483647, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, int64_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::int64_t value;

  // Min NEGATIVE FIXINT.
  value = -64;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max NEGATIVE FIXINT.
  value = -1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::NegativeFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = 127;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I8.
  value = -128;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-128));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I8.
  value = -65;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I8, Integer<std::int8_t>(-65));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I16.
  value = -32768;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(-32768));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I16.
  value = 32767;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(32767));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I32.
  value = -2147483648;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I32, Integer<std::int32_t>(-2147483648));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I32.
  value = 2147483647;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I32, Integer<std::int32_t>(2147483647));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min I64.
  value = -9223372036854775807LL - 1;
  // Believe it or not, this is actually the correct way to specify the most
  // negative signed long long.
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::I64,
                     Integer<std::int64_t>(-9223372036854775807LL - 1));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max I64.
  value = 9223372036854775807LL;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::I64, Integer<std::int64_t>(9223372036854775807LL));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, int64_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::int64_t value;

  // Min NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-64, value);

  // Max NEGATIVE FIXINT.
  reader.Set(Compose(EncodingByte::NegativeFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-1, value);

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(-128)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-128, value);

  // Max I8.
  reader.Set(Compose(EncodingByte::I8, Integer<std::int8_t>(127)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127, value);

  // Min I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(-32768)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-32768, value);

  // Max I16.
  reader.Set(Compose(EncodingByte::I16, Integer<std::int16_t>(32767)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(32767, value);

  // Min I32.
  reader.Set(Compose(EncodingByte::I32, Integer<std::int32_t>(-2147483648)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-2147483648, value);

  // Max I32.
  reader.Set(Compose(EncodingByte::I32, Integer<std::int32_t>(2147483647)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(2147483647, value);

  // Min I64.
  reader.Set(Compose(EncodingByte::I64,
                     Integer<std::int64_t>(-9223372036854775807LL - 1)));
  // Believe it or not, this is actually the correct way to specify the most
  // negative signed long long.
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(-9223372036854775807LL - 1, value);

  // Max I64.
  reader.Set(
      Compose(EncodingByte::I64, Integer<std::int64_t>(9223372036854775807LL)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(9223372036854775807LL, value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, size_t) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  std::size_t value;

  // Min FIXINT.
  value = 0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMin);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max FIXINT.
  value = (1 << 7) - 1;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::PositiveFixIntMax);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U8.
  value = (1 << 7);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, Integer<std::uint8_t>(1 << 7));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U8.
  value = 0xff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U8, 0xff);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U16.
  value = (1 << 8);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(1 << 8));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U16.
  value = 0xffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Min U32.
  value = (1 << 16);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(1 << 16));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U32.
  value = 0xffffffff;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

#if SIZE_MAX == UINT64_MAX
  // Min U64.
  value = (1LLU << 32);
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::U64, Integer<std::uint64_t>(1LLU << 32));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  // Max U64.
  value = 0xffffffffffffffffLLU;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::U64, Integer<std::uint64_t>(0xffffffffffffffffLLU));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
#endif
}

TEST(Deserializer, size_t) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  std::size_t value;

  // Min FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMin));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max FIXINT.
  reader.Set(Compose(EncodingByte::PositiveFixIntMax));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(127U, value);

  // Min U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U8.
  reader.Set(Compose(EncodingByte::U8, Integer<std::uint8_t>(0xff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffU, value);

  // Min U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U16.
  reader.Set(Compose(EncodingByte::U16, Integer<std::uint16_t>(0xffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffU, value);

  // Min U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U32.
  reader.Set(Compose(EncodingByte::U32, Integer<std::uint32_t>(0xffffffff)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffffffU, value);

#if SIZE_MAX == UINT64_MAX
  // Min U64.
  reader.Set(Compose(EncodingByte::U64, Integer<std::uint64_t>(0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0U, value);

  // Max U64.
  reader.Set(Compose(EncodingByte::U64,
                     Integer<std::uint64_t>(0xffffffffffffffffLLU)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0xffffffffffffffffLLU, value);
#endif

  // Test short payload.
  reader.Set(Compose(EncodingByte::U32));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, float) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  float value;

  value = std::numeric_limits<float>::lowest();
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::F32, Float(std::numeric_limits<float>::lowest()));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  value = 0.0f;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::F32, Float(0.0f));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  value = std::numeric_limits<float>::max();
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::F32, Float(std::numeric_limits<float>::max()));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, float) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  float value;

  reader.Set(
      Compose(EncodingByte::F32, Float(std::numeric_limits<float>::lowest())));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<float>::lowest(), value);

  reader.Set(Compose(EncodingByte::F32, Float(0.0f)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0.0f, value);

  reader.Set(
      Compose(EncodingByte::F32, Float(std::numeric_limits<float>::max())));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<float>::max(), value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, double) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;
  double value;

  value = std::numeric_limits<double>::lowest();
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::F64, Float(std::numeric_limits<double>::lowest()));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  value = 0.0;
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected = Compose(EncodingByte::F64, Float(0.0));
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  value = std::numeric_limits<double>::max();
  status = serializer.Write(value);
  ASSERT_TRUE(status);
  expected =
      Compose(EncodingByte::F64, Float(std::numeric_limits<double>::max()));
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(Deserializer, double) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;
  double value;

  reader.Set(
      Compose(EncodingByte::F64, Float(std::numeric_limits<double>::lowest())));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<double>::lowest(), value);

  reader.Set(Compose(EncodingByte::F64, Float(0.0)));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(0.0, value);

  reader.Set(
      Compose(EncodingByte::F64, Float(std::numeric_limits<double>::max())));
  status = deserializer.Read(&value);
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<double>::max(), value);

  // TODO(eieio): Test rejection of all other encoding prefix bytes.
  reader.Set(Compose(EncodingByte::Nil));
  status = deserializer.Read(&value);
  ASSERT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Serializer, StringFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value = "abcd";
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StringFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value = "abcd";
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StringFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value = "abcd";
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StringFailOnWritePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  // Writer::Prepare() indicates write possible and write prefix succeeds but
  // encoding length fails.
  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(4)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::string value = "abcd";
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, StringFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StringFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StringFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::u16string value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidStringLength, status.error());
}

TEST(Deserializer, StringFailOnEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Ensure(Eq(1U)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StringFailOnReadPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Ensure(Eq(1U))).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::string value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, String) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::string value = "abcdefg";

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::String, 7, "abcdefg");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::u16string value = u"abcdefg";

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::String, 7 * sizeof(char16_t), u"abcdefg");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, String) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::string value;

    reader.Set(Compose(EncodingByte::String, 7, "abcdefg"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::string expected = "abcdefg";
    EXPECT_EQ(expected, value);
  }

  {
    std::u16string value;

    reader.Set(Compose(EncodingByte::String, 7 * sizeof(char16_t), u"abcdefg"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::u16string expected = u"abcdefg";
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, VectorString) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    const std::string a = "abcdefg";
    const std::string b = "1234567";
    std::vector<std::string> value = {a, b};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Array, 2, EncodingByte::String, 7,
                       "abcdefg", EncodingByte::String, 7, "1234567");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, VectorString) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::vector<std::string> value;

    reader.Set(Compose(EncodingByte::Array, 2, EncodingByte::String, 7,
                       "abcdefg", EncodingByte::String, 7, "1234567"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    const std::string a = "abcdefg";
    const std::string b = "1234567";
    std::vector<std::string> expected{a, b};
    EXPECT_EQ(expected, value);
  }

  // Test for invalid payload length.
  {
    std::vector<std::string> value;

    // Final string too short.
    reader.Set(Compose(EncodingByte::Array, 2, EncodingByte::String, 7,
                       "abcdefg", EncodingByte::String, 7, "123456"));
    status = deserializer.Read(&value);
    EXPECT_FALSE(status);
    EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());

    // Final array element missing.
    reader.Set(Compose(EncodingByte::Array, 3, EncodingByte::String, 7,
                       "abcdefg", EncodingByte::String, 7, "1234567"));
    status = deserializer.Read(&value);
    EXPECT_FALSE(status);
    EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
  }
}

TEST(Serializer, TupleFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::tuple<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TupleFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::tuple<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TupleFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::tuple<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TupleFailOnWriteElementPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(10))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::tuple<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, TupleFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadSecondElementPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadSecondElementLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadSecondElementEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TupleFailOnReadSecondElementPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::tuple<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Tuple) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::tuple<int, std::string> value(10, "foo");

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected =
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Tuple) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::tuple<int, std::string> value;

    reader.Set(
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::tuple<int, std::string> expected(10, "foo");
    EXPECT_EQ(expected, value);
  }

  // Test for invalid payload length.
  {
    std::tuple<int, std::string> value;

    // String too short.
    reader.Set(
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "fo"));
    status = deserializer.Read(&value);
    EXPECT_FALSE(status);
    EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
  }

  // Test for size mismatch.
  {
    std::tuple<int, std::string> value;

    reader.Set(
        Compose(EncodingByte::Array, 3, 10, EncodingByte::String, 3, "foo"));
    status = deserializer.Read(&value);
    EXPECT_FALSE(status);
    EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
  }
}

TEST(Serializer, PairFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::pair<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, PairFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::pair<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, PairFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::pair<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, PairFailOnWriteElementPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(10))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::pair<int, std::string> value(10, "foo");
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, PairFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadSecondElementPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadSecondElementLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadSecondElementEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, PairFailOnReadSecondElementPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::pair<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Pair) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::pair<int, std::string> value(10, "foo");

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected =
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Pair) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::pair<int, std::string> value;

    reader.Set(
        Compose(EncodingByte::Array, 2, 10, EncodingByte::String, 3, "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::pair<int, std::string> expected(10, "foo");
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, MapFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWriteKeyPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWriteValuePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWriteValueLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, MapFailOnWriteValuePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::map<int, std::string> value{{{0, "abc"}, {1, "123"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadKey) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadValuePrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadValueLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadValueEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, MapFailOnReadKeyPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Map) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::map<int, std::string> value = {{{0, "abc"}, {1, "123"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Map) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::map<int, std::string> value;

    reader.Set(Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::map<int, std::string> expected = {{{0, "abc"}, {1, "123"}}};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, UnorderedMapFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWriteKeyPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWriteValuePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWriteValueLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, UnorderedMapFailOnWriteValuePayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Map)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  std::unordered_map<int, std::string> value{{{0, "abc"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadKey) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadValuePrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadValueLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadValueEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, UnorderedMapFailOnReadKeyPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Map)),
                Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  std::unordered_map<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, UnorderedMap) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    std::unordered_map<int, std::string> value = {{{0, "abc"}, {1, "123"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

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
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    std::unordered_map<int, std::string> value;

    reader.Set(Compose(EncodingByte::Map, 2, 0, EncodingByte::String, 3, "abc",
                       1, EncodingByte::String, 3, "123"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    std::unordered_map<int, std::string> expected = {{{0, "abc"}, {1, "123"}}};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, Enum) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    EnumA value = EnumA::A;

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(1);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::B;

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(127);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::C;

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::U8, 128);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    EnumA value = EnumA::D;

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::U8, 255);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Enum) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    EnumA value;

    reader.Set(Compose(1));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    EnumA expected = EnumA::A;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(127));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    EnumA expected = EnumA::B;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(EncodingByte::U8, 128));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    EnumA expected = EnumA::C;
    EXPECT_EQ(expected, value);
  }

  {
    EnumA value;

    reader.Set(Compose(EncodingByte::U8, 255));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    EnumA expected = EnumA::D;
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, StructureFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestA value{10, "foo"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestA value{10, "foo"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestA value{10, "foo"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteMemberPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(10))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestA value{10, "foo"};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteIntegerLogicalBufferMemberPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestH value{{'a', 'b', 'c'}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteNonIntegerLogicalBufferMemberPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestI value{{"a", "b", "c"}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteIntegerUnboundedLogicalBufferMemberPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<char>({'a', 'b', 'c'});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteNonIntegerUnboundedLogicalBufferMemberPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>({{'a', 'b'}, {'c', 'd'}});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteIntegerLogicalBufferMemberInvalidLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestH value{{'a', 'b', 'c'}, 0xff};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Serializer,
     StructureFailOnWriteNonIntegerLogicalBufferMemberInvalidLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestI value{{"a", "b", "c"}, 6};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Serializer, StructureFailOnWriteIntegerLogicalBufferMemberLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestH value{{'a', 'b', 'c'}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteNonIntegerLogicalBufferMemberLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestI value{{"a", "b", "c"}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteIntegralUnboundedLogicalBufferMemberLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<char>({'a', 'b', 'c'});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteNonIntegralUnboundedLogicalBufferMemberLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>({{'a', 'b'}, {'c', 'd'}});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, StructureFailOnWriteIntegerLogicalBufferMemberPayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestH value{{'a', 'b', 'c'}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteNonIntegerLogicalBufferMemberElementPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TestI value{{"a", "b", "c"}, 3};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteIntegralUnboundedLogicalBufferMemberPayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Binary)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(3)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<char>({'a', 'b', 'c'});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer,
     StructureFailOnWriteNonIntegralUnboundedLogicalBufferMemberPayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Structure)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Array)))
      .Times(2)
      .WillOnce(Return(Status<void>{}))
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>({{'a', 'b'}, {'c', 'd'}});
  status = serializer.Write(*value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidMemberCount, status.error());
}

TEST(Deserializer, StructureFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadSecondElementPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadSecondElementLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadSecondElementEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadSecondElementPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(5))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestA value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegerLogicalBufferPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestH value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegerLogicalBufferPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestI value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegerUnboundedLogicalBufferPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<char>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegerUnboundedLogicalBufferPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadUnboundedLogicalBufferInvalidPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(
                          static_cast<std::uint8_t>(EncodingByte::ReservedMin)),
                      Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<char>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedEncodingType, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegerLogicalBufferLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestH value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegerLogicalBufferLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestI value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegralUnboundedLogicalBufferLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<char>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegralUnboundedLogicalBufferLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegerLogicalBufferInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  auto set_element = [](void* begin, void* /*end*/) {
    std::uint8_t* byte = static_cast<std::uint8_t*>(begin);
    *byte = 0xff;
  };

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::U8)),
                Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(DoAll(Invoke(set_element), Return(Status<void>{})));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestH value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer,
     StructureFailOnReadIntegerUnboundedLogicalBufferInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  auto set_element = [](void* begin, void* /*end*/) {
    const std::size_t value = 1;
    std::copy(&value, &value + 1, static_cast<std::uint8_t*>(begin));
  };

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(
          DoAll(SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::U8)),
                Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(DoAll(Invoke(set_element), Return(Status<void>{})));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<std::uint16_t>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegerLogicalBufferInvalidLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(6), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestI value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidContainerLength, status.error());
}

TEST(Deserializer, StructureFailOnReadIntegerLogicalBufferPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Binary)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(3), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestH value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, StructureFailOnReadNonIntegerLogicalBufferElementPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(5), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(ErrorStatus::ReadLimitReached)))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TestI value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer,
     StructureFailOnReadNonIntegerUnboundedLogicalBufferElementPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Structure)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(5), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Array)),
          Return(ErrorStatus::ReadLimitReached)))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  auto value = MakeTestJ<std::pair<char, char>>(10);
  status = deserializer.Read(value.get());
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Structure) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    TestA value{10, "foo"};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected =
        Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestB value{{10, "foo"}, EnumA::C};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected =
        Compose(EncodingByte::Structure, 2, EncodingByte::Structure, 2, 10,
                EncodingByte::String, 3, "foo", EncodingByte::U8, 128);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestC value;

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Structure, 0);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestD value{10, EnumA::A, "foo"};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Structure, 3, 10, 1, EncodingByte::String,
                       3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestE<int> value_a{10, {1, 2, 3}};
    TestE<std::string> value_b{"foo", {"bar", "baz", "fuz"}};

    ASSERT_TRUE(serializer.Write(value_a));
    ASSERT_TRUE(serializer.Write(value_b));

    expected = Compose(EncodingByte::Structure, 2, 10, EncodingByte::Binary,
                       3 * sizeof(int), Integer<int>(1), Integer<int>(2),
                       Integer<int>(3), EncodingByte::Structure, 2,
                       EncodingByte::String, 3, "foo", EncodingByte::Array, 3,
                       EncodingByte::String, 3, "bar", EncodingByte::String, 3,
                       "baz", EncodingByte::String, 3, "fuz");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestF<int, std::string> value{10, "foo"};

    ASSERT_TRUE(serializer.Write(value));

    expected =
        Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestG value{10, {20, "foo"}};

    ASSERT_TRUE(serializer.Write(value));

    expected = Compose(EncodingByte::Structure, 2, 10, EncodingByte::Structure,
                       2, 20, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestH value{{1, 2, 3}, 3};

    ASSERT_TRUE(serializer.Write(value));

    expected =
        Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 3, 1, 2, 3);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestH2 value{{{1, 2, 3}}, 3};

    ASSERT_TRUE(serializer.Write(value));

    expected =
        Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 3, 1, 2, 3);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TestI value{{"abc", "xyzw"}, 2};

    ASSERT_TRUE(serializer.Write(value));

    expected = Compose(EncodingByte::Structure, 1, EncodingByte::Array, 2,
                       EncodingByte::String, 3, "abc", EncodingByte::String, 4,
                       "xyzw");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    auto value = MakeTestJ<char>({'a', 'b', 'c', 'd'});

    ASSERT_TRUE(serializer.Write(*value));

    expected = Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 4, 'a',
                       'b', 'c', 'd');
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    auto value = MakeTestJ<std::pair<char, char>>({{'a', 'b'}, {'c', 'd'}});

    ASSERT_TRUE(serializer.Write(*value));

    expected = Compose(EncodingByte::Structure, 1, EncodingByte::Array, 2,
                       EncodingByte::Array, 2, 'a', 'b', EncodingByte::Array, 2,
                       'c', 'd');
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Structure) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    TestA value;

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3,
                       "foo"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TestA expected{10, "foo"};
    EXPECT_EQ(expected, value);
  }

  {
    TestB value;

    reader.Set(Compose(EncodingByte::Structure, 2, EncodingByte::Structure, 2,
                       10, EncodingByte::String, 3, "foo", EncodingByte::U8,
                       128));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TestB expected{{10, "foo"}, EnumA::C};
    EXPECT_EQ(expected, value);
  }

  {
    TestC value;

    reader.Set(Compose(EncodingByte::Structure, 0));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);
  }

  {
    TestD value;

    reader.Set(Compose(EncodingByte::Structure, 3, 10, 1, EncodingByte::String,
                       3, "foo"));
    ASSERT_TRUE(deserializer.Read(&value));

    TestD expected{10, EnumA::A, "foo"};
    EXPECT_EQ(expected, value);
  }

  {
    TestE<int> value_a;
    TestE<std::string> value_b;

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::Binary,
                       3 * sizeof(int), Integer<int>(1), Integer<int>(2),
                       Integer<int>(3), EncodingByte::Structure, 2,
                       EncodingByte::String, 3, "foo", EncodingByte::Array, 3,
                       EncodingByte::String, 3, "bar", EncodingByte::String, 3,
                       "baz", EncodingByte::String, 3, "fuz"));
    ASSERT_TRUE(deserializer.Read(&value_a));
    ASSERT_TRUE(deserializer.Read(&value_b));

    TestE<int> expected_a{10, {1, 2, 3}};
    TestE<std::string> expected_b{"foo", {"bar", "baz", "fuz"}};
    EXPECT_EQ(expected_a, value_a);
    EXPECT_EQ(expected_b, value_b);
  }

  {
    TestF<int, std::string> value;

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3,
                       "foo"));
    ASSERT_TRUE(deserializer.Read(&value));

    TestF<int, std::string> expected{10, "foo"};
    EXPECT_EQ(expected, value);
  }

  {
    TestG value;

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::Structure,
                       2, 20, EncodingByte::String, 3, "foo"));
    ASSERT_TRUE(deserializer.Read(&value));

    TestG expected{10, {20, "foo"}};
    EXPECT_EQ(expected, value);
  }

  {
    TestH value{{0}, 0};

    reader.Set(
        Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 3, 1, 2, 3));
    ASSERT_TRUE(deserializer.Read(&value));

    TestH expected{{1, 2, 3}, 3};
    EXPECT_EQ(0, std::memcmp(&expected, &value, sizeof(TestH)));
  }

  {
    TestH2 value{{{0}}, 0};

    reader.Set(
        Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 3, 1, 2, 3));
    ASSERT_TRUE(deserializer.Read(&value));

    TestH2 expected{{{1, 2, 3}}, 3};
    EXPECT_EQ(0, std::memcmp(&expected, &value, sizeof(TestH2)));
  }

  {
    TestI value{{}, 0};

    reader.Set(Compose(EncodingByte::Structure, 1, EncodingByte::Array, 2,
                       EncodingByte::String, 3, "abc", EncodingByte::String, 4,
                       "xyzw"));
    ASSERT_TRUE(deserializer.Read(&value));

    TestI expected{{"abc", "xyzw"}, 2};
    EXPECT_EQ(expected, value);
  }

  {
    auto value = MakeTestJ<char>(10);

    reader.Set(Compose(EncodingByte::Structure, 1, EncodingByte::Binary, 4, 'a',
                       'b', 'c', 'd'));
    ASSERT_TRUE(deserializer.Read(value.get()));

    const char expected[4] = {'a', 'b', 'c', 'd'};
    ASSERT_EQ(4u, value->size);
    EXPECT_EQ(0, std::memcmp(expected, value->data, value->size));
  }

  {
    auto value = MakeTestJ<std::pair<char, char>>(10);

    reader.Set(Compose(EncodingByte::Structure, 1, EncodingByte::Array, 2,
                       EncodingByte::Array, 2, 'a', 'b', EncodingByte::Array, 2,
                       'c', 'd'));
    ASSERT_TRUE(deserializer.Read(value.get()));

    const std::pair<char, char> expected[2] = {{'a', 'b'}, {'c', 'd'}};
    ASSERT_EQ(2u, value->size);
    EXPECT_EQ(0, std::memcmp(expected, value->data,
                             value->size * sizeof(expected[0])));
  }
}

TEST(Serializer, TableFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteHash) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteEntryIdPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteEntryLengthPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(13))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteEntryPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(13)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteEntryStringLength) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(13)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(11))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableFailOnWriteEntryStringPayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(2)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(13)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(11)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TableA2FailOnWriteEntryStringPayload) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Table)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(1)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(13)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::String)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(11)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Skip(_, _)).Times(1);

  TableA2 value{"Ron Swanson"};
  status = serializer.Write(value);
  EXPECT_TRUE(status);
}

TEST(Deserializer, TableFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadHash) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadInvalidHash) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(32), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidTableHash, status.error());
}

TEST(Deserializer, TableFailOnReadLengthPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryId) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryStringLength) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryPayloadEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(11))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(11), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadEntryPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(11)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(11), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnReadDuplicateEntry) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(11)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(11), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(reader, Skip(0)).Times(1);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::DuplicateTableEntry, status.error());
}

TEST(Deserializer, TableFailOnSkipEntryReadSize) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, TableFailOnSkipEntry) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Table)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(10), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(13), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(13))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));

  TableA1 value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Table) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};
  Status<void> status;

  {
    TableA1 value{"Ron Swanson", {{"snarky", "male", "attitude"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Table, 15, 2, 0, 13, EncodingByte::String,
                       11, "Ron Swanson", 1, 26, EncodingByte::Array, 3,
                       EncodingByte::String, 6, "snarky", EncodingByte::String,
                       4, "male", EncodingByte::String, 8, "attitude");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TableA1 value{{{"snarky", "male", "attitude"}}};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected =
        Compose(EncodingByte::Table, 15, 1, 1, 26, EncodingByte::Array, 3,
                EncodingByte::String, 6, "snarky", EncodingByte::String, 4,
                "male", EncodingByte::String, 8, "attitude");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    TableA1 value{"Ron Swanson"};

    status = serializer.Write(value);
    ASSERT_TRUE(status);

    expected = Compose(EncodingByte::Table, 15, 1, 0, 13, EncodingByte::String,
                       11, "Ron Swanson");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Table) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    TableA1 value;

    reader.Set(Compose(EncodingByte::Table, 15, 2, 0, 13, EncodingByte::String,
                       11, "Ron Swanson", 1, 26, EncodingByte::Array, 3,
                       EncodingByte::String, 6, "snarky", EncodingByte::String,
                       4, "male", EncodingByte::String, 8, "attitude"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TableA1 expected{"Ron Swanson", {{"snarky", "male", "attitude"}}};
    EXPECT_EQ(expected, value);
  }

  {
    TableA1 value;

    reader.Set(Compose(EncodingByte::Table, 15, 1, 1, 26, EncodingByte::Array,
                       3, EncodingByte::String, 6, "snarky",
                       EncodingByte::String, 4, "male", EncodingByte::String, 8,
                       "attitude"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TableA1 expected{{{"snarky", "male", "attitude"}}};
    EXPECT_EQ(expected, value);
  }

  {
    TableA1 value;

    reader.Set(Compose(EncodingByte::Table, 15, 1, 0, 13, EncodingByte::String,
                       11, "Ron Swanson"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TableA1 expected{"Ron Swanson"};
    EXPECT_EQ(expected, value);
  }

  {
    TableA2 value;

    reader.Set(Compose(EncodingByte::Table, 15, 2, 0, 13, EncodingByte::String,
                       11, "Ron Swanson", 1, 26, EncodingByte::Array, 3,
                       EncodingByte::String, 6, "snarky", EncodingByte::String,
                       4, "male", EncodingByte::String, 8, "attitude"));
    status = deserializer.Read(&value);
    ASSERT_TRUE(status);

    TableA2 expected{"Ron Swanson"};
    EXPECT_EQ(expected, value);
  }
}

TEST(Serializer, VariantFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  Variant<int, std::string> value{10};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, VariantFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Variant)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  Variant<int, std::string> value{10};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, TupleFailOnWriteIdPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Variant)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  Variant<int, std::string> value{10};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, VariantFailOnWriteElementPrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Variant)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(10))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  Variant<int, std::string> value{10};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, VariantFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, VariantFailOnReadIdPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Variant)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, VariantFailOnInvalidId) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Variant)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(2), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedVariantType, status.error());
}

TEST(Deserializer, VariantFailOnReadElement) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Variant)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, VariantFailOnReadSecondElementEnsure) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Variant)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, VariantFailOnReadSecondElementPayload) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  EXPECT_CALL(reader, Ensure(4)).Times(1);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(4))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Variant)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::String)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(4), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(NotNull(), NotNull()))
      .Times(1)
      .WillOnce(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Skip(_)).Times(0);

  Variant<int, std::string> value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Serializer, Variant) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  {
    Variant<int, std::string> value_a{10};
    Variant<int, std::string> value_b{"foo"};
    Variant<int, std::string> value_c;

    ASSERT_TRUE(serializer.Write(value_a));
    ASSERT_TRUE(serializer.Write(value_b));
    ASSERT_TRUE(serializer.Write(value_c));

    expected = Compose(EncodingByte::Variant, 0, 10, EncodingByte::Variant, 1,
                       EncodingByte::String, 3, "foo", EncodingByte::Variant,
                       -1, EncodingByte::Nil);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    std::array<Variant<int, std::string>, 3> value{
        {Variant<int, std::string>(10), Variant<int, std::string>("foo"),
         Variant<int, std::string>()}};

    ASSERT_TRUE(serializer.Write(value));

    expected = Compose(EncodingByte::Array, 3, EncodingByte::Variant, 0, 10,
                       EncodingByte::Variant, 1, EncodingByte::String, 3, "foo",
                       EncodingByte::Variant, -1, EncodingByte::Nil);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Variant) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};

  {
    Variant<int, std::string> value_a;
    Variant<int, std::string> value_b;
    Variant<int, std::string> value_c;

    reader.Set(Compose(EncodingByte::Variant, 0, 10, EncodingByte::Variant, 1,
                       EncodingByte::String, 3, "foo", EncodingByte::Variant,
                       -1, EncodingByte::Nil));

    ASSERT_TRUE(deserializer.Read(&value_a));
    ASSERT_TRUE(deserializer.Read(&value_b));
    ASSERT_TRUE(deserializer.Read(&value_c));

    ASSERT_TRUE(value_a.is<int>());
    EXPECT_EQ(10, std::get<int>(value_a));
    ASSERT_TRUE(value_b.is<std::string>());
    EXPECT_EQ("foo", std::get<std::string>(value_b));
    EXPECT_TRUE(value_c.empty());
  }
}

TEST(Serializer, Value) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  {
    ValueWrapper<int> value_a{10};
    ValueWrapper<std::string> value_b{"foo"};

    ASSERT_TRUE(serializer.Write(value_a));
    ASSERT_TRUE(serializer.Write(value_b));

    expected = Compose(10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    ArrayWrapper<std::string, 3> value{{{"foo", "bar", "baz"}}, 2};

    ASSERT_TRUE(serializer.Write(value));

    expected = Compose(EncodingByte::Array, 2, EncodingByte::String, 3, "foo",
                       EncodingByte::String, 3, "bar");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    auto value = MakeCDynamicArray<char>({'a', 'b', 'c', 'd'});

    ASSERT_TRUE(serializer.Write(*value));

    expected = Compose(EncodingByte::Binary, 4, 'a', 'b', 'c', 'd');
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, Value) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};

  {
    ValueWrapper<int> value_a;
    ValueWrapper<std::string> value_b;

    reader.Set(Compose(10, EncodingByte::String, 3, "foo"));

    ASSERT_TRUE(deserializer.Read(&value_a));
    ASSERT_TRUE(deserializer.Read(&value_b));

    EXPECT_EQ(10, value_a.value);
    EXPECT_EQ("foo", value_b.value);
  }

  {
    ArrayWrapper<std::string, 3> value;

    reader.Set(Compose(EncodingByte::Array, 2, EncodingByte::String, 3, "foo",
                       EncodingByte::String, 3, "bar"));

    ASSERT_TRUE(deserializer.Read(&value));

    const std::array<std::string, 3> expected{{"foo", "bar"}};
    EXPECT_EQ(expected, value.data);
    EXPECT_EQ(2u, value.size);
  }

  {
    auto value = MakeCDynamicArray<char>(10);

    reader.Set(Compose(EncodingByte::Binary, 4, 'a', 'b', 'c', 'd'));

    ASSERT_TRUE(deserializer.Read(value.get()));

    const char expected[4] = {'a', 'b', 'c', 'd'};
    ASSERT_EQ(4u, value->size);
    EXPECT_EQ(0, std::memcmp(expected, value->data, value->size));
  }
}

TEST(Serializer, HandleFailOnPrepare) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_)).Times(0);
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  IntHandle value{1};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, HandleFailOnWritePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Handle)))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  IntHandle value{1};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, HandleFailOnWriteTypePrefix) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Handle)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  IntHandle value{1};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Serializer, HandleFailOnPushHandle) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Handle)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, PushIntHandle(1))
      .Times(1)
      .WillOnce(Return(ErrorStatus::InvalidHandleValue));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  IntHandle value{1};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidHandleValue, status.error());
}

TEST(Serializer, HandleFailOnWriteHandleReference) {
  MockWriter writer;
  Serializer<MockWriter*> serializer{&writer};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(writer, Prepare(Gt(0U)))
      .Times(AtLeast(1))
      .WillOnce(Return(Status<void>{}))
      .WillRepeatedly(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, Write(static_cast<std::uint8_t>(EncodingByte::Handle)))
      .Times(1)
      .WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(0)).Times(1).WillOnce(Return(Status<void>{}));
  EXPECT_CALL(writer, Write(15))
      .Times(1)
      .WillOnce(Return(ErrorStatus::WriteLimitReached));
  EXPECT_CALL(writer, PushIntHandle(1)).Times(1).WillOnce(Return(15));
  EXPECT_CALL(writer, Write(_, _)).Times(0);
  EXPECT_CALL(writer, Skip(_, _)).Times(0);

  IntHandle value{1};
  status = serializer.Write(value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::WriteLimitReached, status.error());
}

TEST(Deserializer, HandleFailOnReadPrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  IntHandle value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, HandleFailOnReadTypePrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Handle)),
          Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  IntHandle value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, HandleFailOnReadInvalidType) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(2))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Handle)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  IntHandle value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::UnexpectedHandleType, status.error());
}

TEST(Deserializer, HandleFailOnReadReferencePrefix) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Handle)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  IntHandle value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::ReadLimitReached, status.error());
}

TEST(Deserializer, HandleFailOnGetHandle) {
  MockReader reader;
  Deserializer<MockReader*> deserializer{&reader};
  Status<void> status;

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;

  EXPECT_CALL(reader, Ensure(_)).Times(0);
  EXPECT_CALL(reader, Read(_))
      .Times(AtLeast(3))
      .WillOnce(DoAll(
          SetArgPointee<0>(static_cast<std::uint8_t>(EncodingByte::Handle)),
          Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(0), Return(Status<void>{})))
      .WillOnce(DoAll(SetArgPointee<0>(15), Return(Status<void>{})))
      .WillRepeatedly(Return(ErrorStatus::ReadLimitReached));
  EXPECT_CALL(reader, GetIntHandle(15))
      .Times(1)
      .WillOnce(Return(ErrorStatus::InvalidHandleReference));
  EXPECT_CALL(reader, Read(_, _)).Times(0);
  EXPECT_CALL(reader, Skip(_)).Times(0);

  IntHandle value;
  status = deserializer.Read(&value);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidHandleReference, status.error());
}

TEST(Serializer, Handle) {
  std::vector<std::uint8_t> expected;
  std::vector<int> expected_handles;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;
  const auto handle_type = IntHandlePolicy::HandleType();

  {
    IntHandle handle{1};
    EXPECT_TRUE(handle);

    ASSERT_TRUE(serializer.Write(handle));

    expected = Compose(EncodingByte::Handle, handle_type, 0);
    expected_handles = {handle.get()};
    EXPECT_EQ(expected, writer.data());
    EXPECT_EQ(expected_handles, writer.handles());
    writer.clear();
  }

  {
    IntHandle handle_a{1};
    IntHandle handle_b{2};
    IntHandle handle_c;
    EXPECT_TRUE(handle_a);
    EXPECT_TRUE(handle_b);
    EXPECT_FALSE(handle_c);

    ASSERT_TRUE(serializer.Write(handle_a));
    ASSERT_TRUE(serializer.Write(handle_b));
    ASSERT_TRUE(serializer.Write(handle_c));

    expected =
        Compose(EncodingByte::Handle, handle_type, 0, EncodingByte::Handle,
                handle_type, 1, EncodingByte::Handle, handle_type, -1);
    expected_handles = {handle_a.get(), handle_b.get()};
    EXPECT_EQ(expected, writer.data());
    EXPECT_EQ(expected_handles, writer.handles());
  }
}

TEST(Deserializer, Handle) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};

  using IntHandlePolicy = DefaultHandlePolicy<int, -1>;
  using IntHandle = Handle<IntHandlePolicy>;
  const auto handle_type = IntHandlePolicy::HandleType();

  {
    IntHandle handle;

    reader.Set(Compose(EncodingByte::Handle, handle_type, 0));
    reader.SetHandles({1});
    ASSERT_TRUE(deserializer.Read(&handle));

    EXPECT_TRUE(handle);
    EXPECT_EQ(1, handle.get());
  }

  {
    IntHandle handle_a;
    IntHandle handle_b;
    IntHandle handle_c;

    reader.Set(Compose(EncodingByte::Handle, handle_type, 0,
                       EncodingByte::Handle, handle_type, 1,
                       EncodingByte::Handle, handle_type, -1));
    reader.SetHandles({1, 2});
    ASSERT_TRUE(deserializer.Read(&handle_a));
    ASSERT_TRUE(deserializer.Read(&handle_b));
    ASSERT_TRUE(deserializer.Read(&handle_c));

    EXPECT_TRUE(handle_a);
    EXPECT_EQ(1, handle_a.get());
    EXPECT_TRUE(handle_b);
    EXPECT_EQ(2, handle_b.get());
    EXPECT_FALSE(handle_c);
  }
}

TEST(Serializer, reference_wrapper) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  {
    TestA value{10, "foo"};
    auto ref_value = std::ref(value);

    ASSERT_TRUE(serializer.Write(ref_value));

    expected =
        Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}

TEST(Deserializer, reference_wrapper) {
  TestReader reader;
  Deserializer<TestReader*> deserializer{&reader};
  Status<void> status;

  {
    TestA value;
    auto ref_value = std::ref(value);

    reader.Set(Compose(EncodingByte::Structure, 2, 10, EncodingByte::String, 3,
                       "foo"));
    status = deserializer.Read(&ref_value);
    ASSERT_TRUE(status);

    TestA expected{10, "foo"};
    EXPECT_EQ(expected, value);
  }
}
