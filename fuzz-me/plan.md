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
    
[x] use radamsa to fuzz it - **Mutation-based Fuzzing Complete** ✅
    - **Tool**: Radamsa with custom bash wrapper script (`radamsa/run_fuzzer.sh`)
    - **Approach**: Black-box mutation fuzzing with realistic IoT seed inputs
    - **Features**: 
        - 8 realistic MQTT/HTTP seed messages
        - Comprehensive logging (crashes, timeouts, errors)
        - Configurable runs (-n) and timeout (-t) options
        - Automatic input categorization and storage
        - AddressSanitizer/UBSanitizer integration
    - **Results Achieved**: 
        - Successfully found buffer overflow (490 bytes → 256-byte buffer)
        - Triggered intentional vulnerability in `parse_mqtt_topic()` function
        - Generated ASan report with exact location and stack trace
        - Demonstrated practical fuzzing workflow: Generate → Test → Log → Analyze
    - **Key Insights**: 
        - Quick setup and immediate results (< 1000 iterations)
        - No source code knowledge required (black-box)
        - Sanitizers essential for catching subtle memory bugs
        - Mutation diversity effective for finding input validation issues
    - **Location**: `radamsa/` directory with complete documentation and examples

[ ] use boofuzz for protocol-aware fuzzing
    - **Tool**: boofuzz - network protocol fuzzer and protocol analysis toolkit
    - **Approach**: Protocol-aware stateful fuzzing with structured input generation
    - **Target**: IoT parser via network interface or stdin simulation
    - **Features**:
        - Protocol modeling and state machines
        - Intelligent field fuzzing (vs pure mutation)
        - Session management and monitoring
        - Crash detection and logging
        - Web UI for monitoring and analysis
    - **Comparison**: Structure-aware vs radamsa's blind mutations
    - **Location**: `boofuzz/` directory

