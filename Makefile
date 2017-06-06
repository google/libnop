what_to_build:: all

-include local.mk

TOOLCHAIN ?=

HOST_OS := $(shell uname -s)

HOST_CFLAGS := -g -O2 -Wall -I/opt/local/include -Iinclude
HOST_CXXFLAGS := -std=c++14
HOST_LDFLAGS := -L/opt/local/lib -lgtest -lgtest_main

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

include build/host-executable.mk

clean::
	@echo clean
	@rm -rf $(OUT)

all:: $(ALL)

# we generate .d as a side-effect of compiling. override generic rule:
%.d:
-include $(DEPS)
