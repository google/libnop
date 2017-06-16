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
}
