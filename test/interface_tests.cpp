#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <vector>

#include <nop/rpc/interface.h>
#include <nop/serializer.h>

#include "test_reader.h"
#include "test_utilities.h"
#include "test_writer.h"

using nop::BindInterface;
using nop::Compose;
using nop::Deserializer;
using nop::EncodingByte;
using nop::ErrorStatus;
using nop::Float;
using nop::Integer;
using nop::Interface;
using nop::TestReader;
using nop::TestWriter;
using nop::Serializer;
using nop::Variant;

namespace {

constexpr char kTestInterfaceName[] = "io.github.eieio.TestInterface";

struct MessageA {
  int a;
  std::string b;

  NOP_MEMBERS(MessageA, a, b);
};

struct MessageB {
  float a;
  std::vector<int> b;

  NOP_MEMBERS(MessageB, a, b);
};

struct TestInterface : Interface<TestInterface> {
  NOP_INTERFACE(kTestInterfaceName);

  NOP_METHOD(Sum, int(int a, int b));
  NOP_METHOD(Product, int(int a, int b));
  NOP_METHOD(Length, std::size_t(const std::string& string));
  NOP_METHOD(Match, bool(const Variant<MessageA, MessageB>& message));

  NOP_API(Sum, Product, Match);
};

}  // anonymous namespace

TEST(InterfaceTests, Interface) {
  EXPECT_EQ(kTestInterfaceName, TestInterface::GetInterfaceName());
  EXPECT_EQ(TestInterface::Sum::Hash, TestInterface::GetMethodHash<0>());
  EXPECT_EQ(TestInterface::Product::Hash, TestInterface::GetMethodHash<1>());
  EXPECT_EQ(TestInterface::Match::Hash, TestInterface::GetMethodHash<2>());
}

TEST(InterfaceTests, Bind) {
  std::vector<std::uint8_t> expected;
  TestReader reader;
  TestWriter writer;
  Deserializer<TestReader*> deserializer{&reader};
  Serializer<TestWriter*> serializer{&writer};

  auto binding = BindInterface(
      TestInterface::Sum::Bind([](int a, int b) { return a + b; }),
      TestInterface::Product::Bind([](int a, int b) { return a * b; }));
  EXPECT_EQ(2u, binding.Count);

  // Methods not bound should return an error.
  reader.Set(Compose(EncodingByte::U64,
                     Integer<std::uint64_t>(TestInterface::Match::Hash)));
  auto status = binding.Dispatch(&deserializer, &serializer);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidInterfaceMethod, status.error());

  reader.Set(Compose(EncodingByte::U64,
                     Integer<std::uint64_t>(TestInterface::Sum::Hash),
                     EncodingByte::Array, 2, 10, 20));
  expected = Compose(30);

  status = binding.Dispatch(&deserializer, &serializer);
  ASSERT_TRUE(status);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  reader.Set(Compose(EncodingByte::U64,
                     Integer<std::uint64_t>(TestInterface::Product::Hash),
                     EncodingByte::Array, 2, 10, 20));
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(200));

  status = binding.Dispatch(&deserializer, &serializer);
  ASSERT_TRUE(status);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(InterfaceTests, Invoke) {
  std::vector<std::uint8_t> expected;
  TestWriter writer;
  Serializer<TestWriter*> serializer{&writer};

  {
    ASSERT_TRUE((TestInterface::Sum::Invoke(&serializer, 10, 20)));

    expected = Compose(EncodingByte::U64,
                       Integer<std::uint64_t>(TestInterface::Sum::Hash),
                       EncodingByte::Array, 2, 10, 20);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    ASSERT_TRUE((TestInterface::Length::Invoke(&serializer, "foo")));

    expected = Compose(EncodingByte::U64,
                       Integer<std::uint64_t>(TestInterface::Length::Hash),
                       EncodingByte::Array, 1, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    Variant<MessageA, MessageB> message_a{MessageA{10, "foo"}};
    Variant<MessageA, MessageB> message_b{MessageB{20.0f, {1, 2, 3}}};
    ASSERT_TRUE((TestInterface::Match::Invoke(&serializer, message_a)));
    ASSERT_TRUE((TestInterface::Match::Invoke(&serializer, message_b)));

    expected = Compose(
        EncodingByte::U64, Integer<std::uint64_t>(TestInterface::Match::Hash),
        EncodingByte::Array, 1, EncodingByte::Variant, 0,
        EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo",
        EncodingByte::U64, Integer<std::uint64_t>(TestInterface::Match::Hash),
        EncodingByte::Array, 1, EncodingByte::Variant, 1,
        EncodingByte::Structure, 2, EncodingByte::F32, Float(20.0f),
        EncodingByte::Binary, 3 * sizeof(int), Integer<int>(1), Integer<int>(2),
        Integer<int>(3));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}
