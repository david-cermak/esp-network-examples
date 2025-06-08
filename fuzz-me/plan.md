# Fuzzing experiments

## Goal
- fuzzing a naive C program
- fuzzing a real life library (MQTT, mdns, lwip for ESP)
- methods:
    - use coverage guided fuzzers
    - use stateful (protocol) fuzzing
    - fuzzing on target

## Plan 

[x] create a simple C program - **IoT Network Message Parser** ✅
    - **What**: Realistic IoT message parser handling MQTT (`mqtt/topic/path {"key":"value"}`) and HTTP (`GET/POST /path {"key":"value"}`) with JSON payload extraction
    - **Built with**: CMake build system with optional ASAN/UBSAN flags (disabled by default)
    - **Intentional bugs**: 6 subtle vulnerabilities perfect for fuzzing:
        1. Buffer overflow in MQTT topic parsing (strcpy without bounds checking)
        2. Off-by-one errors in string manipulation  
        3. Integer overflow in JSON numeric parsing
        4. Use-after-free in HTTP request parsing
        5. Format string vulnerability in error messages
        6. Null pointer dereference with malformed input
    - **Fuzzing-ready**: Reads from stdin, single file, sanitizers catch bugs immediately
    - **Location**: `sample/iot_parser.c` with full documentation and build instructions
    
[ ] create a fuzz target
[ ] use radamsa to fuzz it


