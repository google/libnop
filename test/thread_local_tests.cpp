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

#include <thread>

#include <nop/types/thread_local.h>

using nop::ThreadLocal;
using nop::ThreadLocalSlot;
using nop::ThreadLocalTypeSlot;

TEST(ThreadLocal, Slots) {
  // Define a local type to prevent interaction between tests.
  struct LocalTypeA {};

  {
    ThreadLocal<int, ThreadLocalSlot<LocalTypeA, 0>> int_value{1};
    EXPECT_EQ(1, int_value.Get());
  }

  {
    // Same type and slot as previous value. Expect the TLS value to be
    // initialized already.
    ThreadLocal<int, ThreadLocalSlot<LocalTypeA, 0>> int_value{0};
    EXPECT_EQ(1, int_value.Get());
  }

  {
    // Different slot than the previous value.
    ThreadLocal<int, ThreadLocalSlot<LocalTypeA, 1>> int_value{2};
    EXPECT_EQ(2, int_value.Get());
  }

  {
    struct LocalTypeB {};
    ThreadLocal<int, ThreadLocalSlot<LocalTypeB, 0>> int_value_a{3};
    EXPECT_EQ(3, int_value_a.Get());

    ThreadLocal<int, ThreadLocalSlot<LocalTypeB, 1>> int_value_b{4};
    EXPECT_EQ(4, int_value_b.Get());

    EXPECT_NE(&int_value_a.Get(), &int_value_b.Get());
  }
}

TEST(ThreadLocal, Thread) {
  struct LocalType {};
  using IntType = ThreadLocal<int, ThreadLocalSlot<LocalType, 0>>;

  {
    IntType int_value_a{0};
    int* int_pointer = nullptr;

    auto Op = [&int_pointer] {
      IntType int_value_b{0};
      int_pointer = &int_value_b.Get();
    };

    std::thread t{Op};
    t.join();

    EXPECT_NE(int_pointer, &int_value_a.Get());
  }
}

TEST(ThreadLocal, Clear) {
  struct LocalType {};
  using IntType = ThreadLocal<int, ThreadLocalTypeSlot<LocalType>>;

  {
    IntType int_value{1};
    EXPECT_EQ(1, int_value.Get());

    int_value.Clear();
    int_value.Initialize(2);
    EXPECT_EQ(2, int_value.Get());
  }

  {
    IntType int_value{3};
    EXPECT_EQ(2, int_value.Get());
  }
}
