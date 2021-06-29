# GTest and GMock
set(GMOCK_VER "1.11.0")
set(GMOCK_ROOT ${CMAKE_CURRENT_BINARY_DIR}/gmock)
find_package(GMock REQUIRED)

# Helper 'nop_add_test'
macro(nop_add_test test_name test_src)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} ${TARGET_NAME} GTest::Main GTest::GTest GMock::GMock)
    add_test(${test_name} ${test_name})
endmacro()

# Tests
nop_add_test(constexpr_tests constexpr_tests.cpp)
nop_add_test(encoding_tests encoding_tests.cpp)
nop_add_test(endian_tests endian_tests.cpp)
nop_add_test(enum_flags_tests enum_flags_tests.cpp)
nop_add_test(fungible_tests fungible_tests.cpp)
nop_add_test(handle_tests handle_tests.cpp)
nop_add_test(interface_tests interface_tests.cpp)
nop_add_test(nop_tests nop_tests.cpp)
nop_add_test(optional_tests optional_tests.cpp)
nop_add_test(result_tests result_tests.cpp)
nop_add_test(serializer_tests serializer_tests.cpp)
nop_add_test(sip_hash_tests sip_hash_tests.cpp)
nop_add_test(thread_local_tests thread_local_tests.cpp)
nop_add_test(utility_tests utility_tests.cpp)
nop_add_test(variant_tests variant_tests.cpp)
