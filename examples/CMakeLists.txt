# Find threads
find_package(Threads)

# Helper 'nop_add_example'
macro(nop_add_example example_name example_src)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} Threads::Threads ${TARGET_NAME})
endmacro()

# Examples
nop_add_example(stream_example stream.cpp)
nop_add_example(simple_protocol_example simple_protocol.cpp)
nop_add_example(interface_example interface.cpp)
nop_add_example(pipe_example pipe.cpp)
nop_add_example(table_example table.cpp)
nop_add_example(variant_example variant.cpp)

# Shared library example
add_library(shared_protocol SHARED shared.cpp)
target_link_libraries(shared_protocol ${TARGET_NAME})
add_custom_target(shared
    # Command
    python2 ${PROJECT_SOURCE_DIR}/examples/shared.py $<TARGET_FILE:shared_protocol>
    DEPENDS shared_protocol
    VERBATIM
    COMMAND_EXPAND_LISTS
)






