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

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include <nop/rpc/interface.h>
#include <nop/rpc/simple_method_receiver.h>
#include <nop/rpc/simple_method_sender.h>
#include <nop/serializer.h>
#include <nop/structure.h>

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
using nop::InterfaceDispatcher;
using nop::InterfaceType;
using nop::Serializer;
using nop::SimpleMethodReceiver;
using nop::SimpleMethodSender;
using nop::Status;
using nop::TestReader;
using nop::TestWriter;
using nop::Variant;

namespace {

constexpr char kTestInterfaceName[] = "io.github.eieio.TestInterface";

struct MessageA {
  int a;
  std::string b;

  NOP_STRUCTURE(MessageA, a, b);
};

struct MessageB {
  float a;
  std::vector<int> b;

  NOP_STRUCTURE(MessageB, a, b);
};

struct TestInterface : Interface<TestInterface> {
  NOP_INTERFACE(kTestInterfaceName);

  NOP_METHOD(Sum, int(int a, int b));
  NOP_METHOD(Product, int(int a, int b));
  NOP_METHOD(Length, std::size_t(const std::string& string));
  NOP_METHOD(Match, bool(const Variant<MessageA, MessageB>& message));

  NOP_INTERFACE_API(Sum, Product, Length, Match);
};

using MethodSelectorType = InterfaceType<TestInterface>::MethodSelector;

// Define the method selector encoding type based on the type of MethodSelector.
constexpr EncodingByte MethodSelectorEncoding = std::conditional_t<
    std::is_same<MethodSelectorType, std::uint64_t>::value,
    std::integral_constant<EncodingByte, EncodingByte::U64>,
    std::integral_constant<EncodingByte, EncodingByte::U32>>::value;

class AbstractClass : private Interface<AbstractClass> {
 public:
  virtual ~AbstractClass() = default;

  virtual int OnSum(int a, int b) = 0;
  virtual std::size_t OnLength(const std::string& string) = 0;

  static auto Bind() {
    return BindInterface<AbstractClass*>(
        Sum::Bind(&AbstractClass::OnSum),
        Length::Bind(&AbstractClass::OnLength));
  }

  // Methods for generating test data.
  static auto GetSumMethodSelector() { return Sum::Selector; }
  static auto GetLengthMethodSelector() { return Length::Selector; }

  template <std::size_t Index>
  static constexpr auto GetMethodSelector() {
    return BASE::GetMethodSelector<Index>();
  }

 private:
  NOP_INTERFACE("io.github.eieio.AbstractClass");
  NOP_METHOD(Sum, int(int a, int b));
  NOP_METHOD(Length, std::size_t(const std::string& string));
  NOP_INTERFACE_API(Sum, Length);
};

struct ConcreteClass : AbstractClass {
  int OnSum(int a, int b) override { return a + b; }
  std::size_t OnLength(const std::string& string) override {
    return string.size();
  }
};

}  // anonymous namespace

TEST(InterfaceTests, Interface) {
  EXPECT_EQ(kTestInterfaceName, TestInterface::GetInterfaceName());
  EXPECT_EQ(TestInterface::Sum::Selector,
            TestInterface::GetMethodSelector<0>());
  EXPECT_EQ(TestInterface::Product::Selector,
            TestInterface::GetMethodSelector<1>());
  EXPECT_EQ(TestInterface::Length::Selector,
            TestInterface::GetMethodSelector<2>());
  EXPECT_EQ(TestInterface::Match::Selector,
            TestInterface::GetMethodSelector<3>());
}

TEST(InterfaceTests, AbstractClass) {
  std::vector<std::uint8_t> expected;
  TestReader reader;
  TestWriter writer;
  Deserializer<TestReader*> deserializer{&reader};
  Serializer<TestWriter*> serializer{&writer};
  auto receiver = MakeSimpleMethodReceiver(&serializer, &deserializer);
  using DispatcherType =
      InterfaceDispatcher<decltype(receiver), AbstractClass*>;

  DispatcherType dispatcher = AbstractClass::Bind();
  EXPECT_TRUE(dispatcher);

  EXPECT_EQ(AbstractClass::GetMethodSelector<0>(),
            AbstractClass::GetSumMethodSelector());

  reader.Set(Compose(
      MethodSelectorEncoding,
      Integer<MethodSelectorType>(AbstractClass::GetSumMethodSelector()),
      EncodingByte::Array, 2, 10, 20));
  expected = Compose(30);

  ConcreteClass instance;
  auto status = dispatcher(&receiver, &instance);
  ASSERT_TRUE(status);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(InterfaceTests, Bind) {
  std::vector<std::uint8_t> expected;
  TestReader reader;
  TestWriter writer;
  Deserializer<TestReader*> deserializer{&reader};
  Serializer<TestWriter*> serializer{&writer};
  auto receiver = MakeSimpleMethodReceiver(&serializer, &deserializer);
  using DispatcherType = InterfaceDispatcher<decltype(receiver)>;

  auto binding = BindInterface(
      TestInterface::Sum::Bind([](int a, int b) { return a + b; }),
      TestInterface::Product::Bind([](int a, int b) { return a * b; }));
  EXPECT_EQ(2u, binding.Count);

  DispatcherType dispatcher = std::move(binding);
  EXPECT_TRUE(dispatcher);

  // Methods not bound should return an error.
  reader.Set(
      Compose(MethodSelectorEncoding,
              Integer<MethodSelectorType>(TestInterface::Match::Selector)));
  auto status = dispatcher(&receiver);
  EXPECT_FALSE(status);
  EXPECT_EQ(ErrorStatus::InvalidInterfaceMethod, status.error());

  reader.Set(Compose(MethodSelectorEncoding,
                     Integer<MethodSelectorType>(TestInterface::Sum::Selector),
                     EncodingByte::Array, 2, 10, 20));
  expected = Compose(30);

  status = dispatcher(&receiver);
  ASSERT_TRUE(status);
  EXPECT_EQ(expected, writer.data());
  writer.clear();

  reader.Set(
      Compose(MethodSelectorEncoding,
              Integer<MethodSelectorType>(TestInterface::Product::Selector),
              EncodingByte::Array, 2, 10, 20));
  expected = Compose(EncodingByte::I16, Integer<std::int16_t>(200));

  status = dispatcher(&receiver);
  ASSERT_TRUE(status);
  EXPECT_EQ(expected, writer.data());
  writer.clear();
}

TEST(InterfaceTests, Invoke) {
  std::vector<std::uint8_t> expected;
  TestReader reader;
  TestWriter writer;
  Deserializer<TestReader*> deserializer{&reader};
  Serializer<TestWriter*> serializer{&writer};
  auto sender = MakeSimpleMethodSender(&serializer, &deserializer);

  {
    reader.Set(Compose(30));

    ASSERT_TRUE((TestInterface::Sum::Invoke(&sender, 10, 20)));

    expected =
        Compose(MethodSelectorEncoding,
                Integer<MethodSelectorType>(TestInterface::Sum::Selector),
                EncodingByte::Array, 2, 10, 20);
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    reader.Set(Compose(3));

    ASSERT_TRUE((TestInterface::Length::Invoke(&sender, "foo")));

    expected =
        Compose(MethodSelectorEncoding,
                Integer<MethodSelectorType>(TestInterface::Length::Selector),
                EncodingByte::Array, 1, EncodingByte::String, 3, "foo");
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }

  {
    reader.Set(Compose(true, true));

    Variant<MessageA, MessageB> message_a{MessageA{10, "foo"}};
    Variant<MessageA, MessageB> message_b{MessageB{20.0f, {1, 2, 3}}};
    ASSERT_TRUE((TestInterface::Match::Invoke(&sender, message_a)));
    ASSERT_TRUE((TestInterface::Match::Invoke(&sender, message_b)));

    expected =
        Compose(MethodSelectorEncoding,
                Integer<MethodSelectorType>(TestInterface::Match::Selector),
                EncodingByte::Array, 1, EncodingByte::Variant, 0,
                EncodingByte::Structure, 2, 10, EncodingByte::String, 3, "foo",
                MethodSelectorEncoding,
                Integer<MethodSelectorType>(TestInterface::Match::Selector),
                EncodingByte::Array, 1, EncodingByte::Variant, 1,
                EncodingByte::Structure, 2, EncodingByte::F32, Float(20.0f),
                EncodingByte::Binary, 3 * sizeof(int), Integer<int>(1),
                Integer<int>(2), Integer<int>(3));
    EXPECT_EQ(expected, writer.data());
    writer.clear();
  }
}
