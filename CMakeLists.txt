cmake_minimum_required(VERSION 3.2)
set(TARGET_NAME "libnop")
project(${TARGET_NAME} LANGUAGES CXX)

# Set modules path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Build tests
option(LIBNOP_BUILD_TESTS "Build libnop tests" OFF)
# Build examples
option(LIBNOP_BUILD_EXAMPLES "Build libnop examples" OFF)

add_library(${TARGET_NAME} INTERFACE)
# Set C++14 version
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    target_compile_features(${TARGET_NAME} INTERFACE cxx_generic_lambdas)
else()
    target_compile_features(${TARGET_NAME} INTERFACE cxx_std_14)
endif()

target_include_directories(${TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

# Tests
if(LIBNOP_BUILD_TESTS)
    # Add CMake testing infrastructure
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

# Examples
if(LIBNOP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS ${TARGET_NAME}
    EXPORT "${TARGET_NAME}Config"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}")
export(TARGETS
        ${TARGET_NAME}
    FILE "${config_install_dir}/${TARGET_NAME}Config.cmake"
)
install(EXPORT
    "${TARGET_NAME}Config"
    DESTINATION "${config_install_dir}"
)
