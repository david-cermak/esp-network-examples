cmake_minimum_required(VERSION 3.10)
project(iot_parser C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_DEBUG "Enable debug symbols" ON)

# Create the executable
add_executable(iot_parser iot_parser.c)

# Debug flags
if(ENABLE_DEBUG)
    target_compile_options(iot_parser PRIVATE -g)
endif()

# AddressSanitizer flags
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    target_compile_options(iot_parser PRIVATE 
        -fsanitize=address 
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
    )
    target_link_options(iot_parser PRIVATE -fsanitize=address)
endif()

# UndefinedBehaviorSanitizer flags
if(ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
    target_compile_options(iot_parser PRIVATE 
        -fsanitize=undefined
        -fno-sanitize-recover=undefined
    )
    target_link_options(iot_parser PRIVATE -fsanitize=undefined)
endif()

# Compiler warnings
target_compile_options(iot_parser PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
)

# Installation
install(TARGETS iot_parser DESTINATION bin)

# Print build configuration
message(STATUS "Build configuration:")
message(STATUS "  Debug symbols: ${ENABLE_DEBUG}")
message(STATUS "  AddressSanitizer: ${ENABLE_ASAN}")
message(STATUS "  UBSanitizer: ${ENABLE_UBSAN}") 