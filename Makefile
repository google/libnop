what_to_build:: all

-include local.mk

TOOLCHAIN ?=

HOST_OS := $(shell uname -s)

GTEST_INSTALL ?= /opt/local
GTEST_LIB ?= $(GTEST_INSTALL)/lib
GTEST_INCLUDE ?= $(GTEST_INSTALL)/include

HOST_CFLAGS := -g -O2 -Wall -I$(GTEST_INCLUDE) -Iinclude
HOST_CXXFLAGS := -std=c++14
HOST_LDFLAGS := -L$(GTEST_LIB) -lgtest -lgtest_main

OUT := out
OUT_HOST_OBJ := $(OUT)/host-obj

ALL :=
DEPS :=

include build/rules.mk

M_NAME := test
M_OBJS := \
	test/nop_tests.o \
	test/encoding_tests.o \
	test/serializer_tests.o \
	test/utility_tests.o \
	test/variant_tests.o \
	test/handle_tests.o \
	test/thread_local_tests.o \
	test/enum_flags_tests.o \

include build/host-executable.mk

clean::
	@echo clean
	@rm -rf $(OUT)

all:: $(ALL)

# we generate .d as a side-effect of compiling. override generic rule:
%.d:
-include $(DEPS)
