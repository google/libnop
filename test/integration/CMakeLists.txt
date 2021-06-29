cmake_minimum_required(VERSION 3.2)

# Create a project with name 'myapp'
set(TARGET_NAME myapp)
project(integration-test)

# Select whether to test integration using add_subdirectory or find_package
option(TEST_FIND_PACKAGE "Test integration using find_package command" OFF)

# Add libnop using find_package
if(TEST_FIND_PACKAGE)
    find_package(libnop CONFIG REQUIRED)
else()
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../" libnop EXCLUDE_FROM_ALL)
endif()

# Create target dependant on libnop
add_executable(integration-test "../../examples/pipe.cpp")
target_include_directories(integration-test PUBLIC "../../examples/")

target_link_libraries(integration-test PRIVATE libnop)

include(CTest)
enable_testing()
add_test(integration-test integration-test)
