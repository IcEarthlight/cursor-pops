cmake_minimum_required(VERSION 3.10)
project(cursor_pops)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable with consistent naming
add_executable(cursor_pops WIN32
    cursor_pops_args.cpp
    cursor_pops_core.cpp
)

# Set Windows subsystem
if(MSVC)
    set_target_properties(cursor_pops PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
    target_compile_options(cursor_pops PRIVATE /W4)
else()
    target_compile_options(cursor_pops PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules
install(TARGETS cursor_pops
    RUNTIME DESTINATION bin
) 