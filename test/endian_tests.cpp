#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

#include <nop/utility/endian.h>

using nop::HostEndian;

namespace {

template <typename T>
union Bytes {
  enum : std::size_t { N = sizeof(T) };

  Bytes(T value) : value{value} {}
  Bytes(const std::uint8_t (&value)[N]) {
    std::copy(&value[0], &value[N], bytes);
  }

  T value;
  std::uint8_t bytes[N];
};

}  // anonymous namespace

TEST(EndianTests, Little) {
  {
    Bytes<std::uint8_t> little_endian{{0x00}};
    EXPECT_EQ(0x00, HostEndian<std::uint8_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value, HostEndian<std::uint8_t>::ToLittle(0x00));
  }

  {
    Bytes<std::uint16_t> little_endian{{0x00, 0x11}};
    EXPECT_EQ(0x1100,
              HostEndian<std::uint16_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value, HostEndian<std::uint16_t>::ToLittle(0x1100));
  }

  {
    Bytes<std::uint32_t> little_endian{{0x00, 0x11, 0x22, 0x33}};
    EXPECT_EQ(0x33221100,
              HostEndian<std::uint32_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value,
              HostEndian<std::uint32_t>::FromLittle(0x33221100));
  }

  {
    Bytes<std::uint64_t> little_endian{
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}};
    EXPECT_EQ(0x7766554433221100ULL,
              HostEndian<std::uint64_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value,
              HostEndian<std::uint64_t>::ToLittle(0x7766554433221100ULL));
  }

  {
    Bytes<std::int8_t> little_endian{{0x00}};
    EXPECT_EQ(0x00, HostEndian<std::int8_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value, HostEndian<std::int8_t>::ToLittle(0x00));
  }

  {
    Bytes<std::int16_t> little_endian{{0x00, 0x11}};
    EXPECT_EQ(0x1100,
              HostEndian<std::int16_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value, HostEndian<std::int16_t>::ToLittle(0x1100));
  }

  {
    Bytes<std::int32_t> little_endian{{0x00, 0x11, 0x22, 0x33}};
    EXPECT_EQ(0x33221100,
              HostEndian<std::int32_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value,
              HostEndian<std::int32_t>::ToLittle(0x33221100));
  }

  {
    Bytes<std::int64_t> little_endian{
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}};
    EXPECT_EQ(0x7766554433221100LL,
              HostEndian<std::int64_t>::FromLittle(little_endian.value));
    EXPECT_EQ(little_endian.value,
              HostEndian<std::int64_t>::ToLittle(0x7766554433221100LL));
  }
}

TEST(EndianTests, Big) {
  {
    Bytes<std::uint8_t> big_endian{{0x00}};
    EXPECT_EQ(0x00, HostEndian<std::uint8_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value, HostEndian<std::uint8_t>::ToBig(0x00));
  }

  {
    Bytes<std::uint16_t> big_endian{{0x11, 0x00}};
    EXPECT_EQ(0x1100, HostEndian<std::uint16_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value, HostEndian<std::uint16_t>::ToBig(0x1100));
  }

  {
    Bytes<std::uint32_t> big_endian{{0x33, 0x22, 0x11, 0x00}};
    EXPECT_EQ(0x33221100, HostEndian<std::uint32_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value, HostEndian<std::uint32_t>::ToBig(0x33221100));
  }

  {
    Bytes<std::uint64_t> big_endian{
        {0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00}};
    EXPECT_EQ(0x7766554433221100ULL,
              HostEndian<std::uint64_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value,
              HostEndian<std::uint64_t>::ToBig(0x7766554433221100ULL));
  }

  {
    Bytes<std::int8_t> big_endian{{0x00}};
    EXPECT_EQ(0x00, HostEndian<std::int8_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value, HostEndian<std::int8_t>::ToBig(0x00));
  }

  {
    Bytes<std::int16_t> big_endian{{0x11, 0x00}};
    EXPECT_EQ(0x1100, HostEndian<std::int16_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value, HostEndian<std::int16_t>::ToBig(0x1100));
  }

  {
    Bytes<std::int32_t> big_endian{{0x33, 0x22, 0x11, 0x00}};
    EXPECT_EQ(big_endian.value, HostEndian<std::int32_t>::ToBig(0x33221100));
  }

  {
    Bytes<std::int64_t> big_endian{
        {0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00}};
    EXPECT_EQ(0x7766554433221100LL,
              HostEndian<std::int64_t>::FromBig(big_endian.value));
    EXPECT_EQ(big_endian.value,
              HostEndian<std::int64_t>::ToBig(0x7766554433221100LL));
  }
}
