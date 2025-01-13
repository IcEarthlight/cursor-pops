cmake_minimum_required(VERSION 3.10)
project(CursorPops)

# Add Unicode definitions
add_definitions(-DUNICODE -D_UNICODE)

# Set Windows subsystem
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

add_executable(CursorPops WIN32 CursorPops.cpp) 
