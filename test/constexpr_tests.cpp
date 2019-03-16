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

#include <gtest/gtest.h>

#include <iterator>
#include <vector>

#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/table.h>
#include <nop/utility/constexpr_buffer_writer.h>
#include <nop/value.h>

#include "test_utilities.h"

using nop::ConstexprBufferWriter;
using nop::Encoding;
using nop::EncodingByte;
using nop::Entry;
using nop::Integer;
using nop::Serializer;

namespace {

template <typename T, size_t Size>
struct Array {
  T elements[Size];

  constexpr T& operator[](size_t index) { return elements[index]; }
  constexpr const T& operator[](size_t index) const { return elements[index]; }
  constexpr T* data() { return elements; }
  constexpr const T* data() const { return elements; }
  constexpr size_t size() const { return Size; }

  constexpr const T* begin() const { return &elements[0]; }
  constexpr const T* end() const { return &elements[Size]; }

  constexpr bool operator==(const Array& other) const {
    for (size_t i = 0; i < Size; i++) {
      if (elements[i] != other.elements[i])
        return false;
    }
    return true;
  }

  NOP_VALUE(Array, elements);
};

template <typename T>
constexpr size_t EncodingSize(const T& value) {
  return nop::Encoding<T>::Size(value);
}

template <std::size_t Size, typename T>
constexpr auto Serialize(const T& value) {
  Array<std::uint8_t, Size> bytes{{}};
  Serializer<nop::ConstexprBufferWriter> serializer{bytes.data(), bytes.size()};
  auto status = serializer.Write(value);
  return status ? bytes : throw status;
}

struct BasicStruct {
  std::uint8_t a;
  std::uint32_t b;
  NOP_STRUCTURE(BasicStruct, a, b);
};

constexpr BasicStruct kBasicStruct{127, 0xa5a5a5a5};

constexpr auto SerializeBasicStruct() {
  constexpr std::size_t Size = EncodingSize(kBasicStruct);
  return Serialize<Size>(kBasicStruct);
}

constexpr auto kSerializedBasicStruct = SerializeBasicStruct();

constexpr Array<BasicStruct, 3> kBasicStructArray{
    {
        {0, 1},
        {2, 3},
        {4, 5},
    },
};

constexpr auto SerializeBasicStructArray() {
  constexpr std::size_t Size = EncodingSize(kBasicStructArray);
  return Serialize<Size>(kBasicStructArray);
}

constexpr auto kSerializedBasicStructArray = SerializeBasicStructArray();

struct BasicTable {
  Entry<int, 0> a;
  Entry<char, 1> b;
  Entry<Array<char, 10>, 2> c;

  NOP_TABLE(BasicTable, a, b, c);
};

constexpr BasicTable kBasicTable{10, 20, {{{'a', 'b', 'c', 'd', 'e'}}}};

static_assert(Encoding<int>::Size(kBasicTable.a.get()) == 1, "");
static_assert(Encoding<char>::Size(kBasicTable.b.get()) == 1, "");
static_assert(Encoding<Array<char, 10>>::Size(kBasicTable.c.get()) == 12, "");
static_assert(EncodingSize(kBasicTable) == 23, "");

constexpr auto SerializeBasicTable() {
  constexpr std::size_t Size = EncodingSize(kBasicTable);
  return Serialize<Size>(kBasicTable);
}

constexpr auto kSerializedBasicTable = SerializeBasicTable();

constexpr Array<BasicTable, 4> kBasicTableArray{
    {
        {10, 20, {{{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'}}}},
        {128, 128, {{{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}}}},
        {255, 255, {{{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}}}},
        {},
    },
};

constexpr auto SerializeBasicTableArray() {
  constexpr std::size_t Size = EncodingSize(kBasicTableArray);
  return Serialize<Size>(kBasicTableArray);
}

constexpr auto kSerializedBasicTableArray = SerializeBasicTableArray();

}  // anonymous namespace

TEST(Constexpr, SerializedData) {
  std::vector<std::uint8_t> expected;
  std::vector<std::uint8_t> actual;

  {
    expected = Compose(EncodingByte::Structure, 2, 127, EncodingByte::U32,
                       Integer<std::uint32_t>(0xa5a5a5a5));
    actual = {std::begin(kSerializedBasicStruct),
              std::end(kSerializedBasicStruct)};
    EXPECT_EQ(expected, actual);
  }

  {
    expected = Compose(EncodingByte::Array, 3, EncodingByte::Structure, 2, 0, 1,
                       EncodingByte::Structure, 2, 2, 3,
                       EncodingByte::Structure, 2, 4, 5);
    actual = {std::begin(kSerializedBasicStructArray),
              std::end(kSerializedBasicStructArray)};
    EXPECT_EQ(expected, actual);
  }

  {
    expected = Compose(EncodingByte::Table, 0, 3, /* entry id 0 */ 0, 1, 10,
                       /* entry id 1 */ 1, 1, 20, /* entry id 2 */ 2, 12,
                       EncodingByte::Binary, 10, 'a', 'b', 'c', 'd', 'e', 0, 0,
                       0, 0, 0);
    actual = {std::begin(kSerializedBasicTable),
              std::end(kSerializedBasicTable)};
    EXPECT_EQ(expected, actual);
  }

  {
    expected = Compose(
        EncodingByte::Array, 4, EncodingByte::Table, 0, 3,
        /* entry id 0 */ 0, 1, 10,
        /* entry id 1 */ 1, 1, 20, /* entry id 2 */ 2, 12, EncodingByte::Binary,
        10, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        EncodingByte::Table, 0, 3,
        /* entry id 0 */ 0, 3, EncodingByte::I16, Integer<std::int16_t>(128),
        /* entry id 1 */ 1, 2, EncodingByte::U8, 128,
        /* entry id 2 */ 2, 12, EncodingByte::Binary, 10, '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', EncodingByte::Table, 0, 3,
        /* entry id 0 */ 0, 3, EncodingByte::I16, Integer<std::int16_t>(255),
        /* entry id 1 */ 1, 2, EncodingByte::U8, 255,
        /* entry id 2 */ 2, 12, EncodingByte::Binary, 10, '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', EncodingByte::Table, 0, 0);
    actual = {std::begin(kSerializedBasicTableArray),
              std::end(kSerializedBasicTableArray)};
    EXPECT_EQ(expected, actual);
  }
}
