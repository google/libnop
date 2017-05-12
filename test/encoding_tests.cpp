#include <gtest/gtest.h>

#include <nop/base/encoding.h>
#include <nop/base/types.h>

using nop::Encoding;
using nop::EncodingType;
using nop::EncodingPrefix;

TEST(Encoding, Boolean) {
  Encoding true_encoding{true};
  EXPECT_EQ(static_cast<EncodingType>(EncodingPrefix::True),
            true_encoding.value());

  Encoding false_encoding{false};
  EXPECT_EQ(static_cast<EncodingType>(EncodingPrefix::False),
            false_encoding.value());
}
