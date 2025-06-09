cmake_minimum_required(VERSION 3.16)
project(iot_parser_fuzztest)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# Clone FuzzTest if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fuzztest/.git")
    message(STATUS "Cloning FuzzTest...")
    execute_process(
        COMMAND git clone https://github.com/google/fuzztest.git
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone FuzzTest")
    endif()
endif()

add_subdirectory(fuzztest)

enable_testing()
include(GoogleTest)

# Setup fuzzing flags before building test targets
fuzztest_setup_fuzzing_flags()

# Create a library from the IoT parser functions
add_library(iot_parser_lib STATIC
    ../sample/iot_parser.c
)

target_include_directories(iot_parser_lib PUBLIC
    ../sample
)

# Define macro to disable main() function when building for fuzz testing
target_compile_definitions(iot_parser_lib PRIVATE
    FUZZTEST_BUILD
)

# Create the fuzz test executable
add_executable(
    iot_parser_fuzztest
    iot_parser_fuzztest.cc
)

target_link_libraries(iot_parser_fuzztest PRIVATE 
    iot_parser_lib
)

link_fuzztest(iot_parser_fuzztest)
# gtest_discover_tests(iot_parser_fuzztest)
gtest_add_tests(TARGET iot_parser_fuzztest)