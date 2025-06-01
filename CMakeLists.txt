cmake_minimum_required(VERSION 3.14)
project(inlining_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(inlining code.cpp)

# set_target_properties(inlining PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Handle FORCE_INLINE and NO_INLINE flags via CMake options
option(FORCE_INLINE "Enable forced inlining via __attribute__((always_inline))" OFF)
option(NO_INLINE "Disable inlining via __attribute__((noinline))" OFF)

# Append appropriate macro definitions based on options
if(FORCE_INLINE)
    target_compile_definitions(inlining PRIVATE FORCE_INLINE)
elseif(NO_INLINE)
    target_compile_definitions(inlining PRIVATE NO_INLINE)
endif()

message(STATUS "FORCE_INLINE = ${FORCE_INLINE}")
message(STATUS "NO_INLINE = ${NO_INLINE}")
