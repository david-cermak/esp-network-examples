# FuzzTest Integration for IoT Parser

This directory contains a complete **FuzzTest** integration for the IoT Network Message Parser, demonstrating modern property-based and coverage-guided fuzzing using Google's FuzzTest framework.

## Overview

**FuzzTest** is Google's C++ fuzzing framework that combines:
- Property-based testing with structured input generation
- Coverage-guided fuzzing for deep exploration
- Integration with GoogleTest for unified testing
- Multiple execution modes (unit testing vs fuzzing)
- Advanced input domains and constraints

## Project Structure

```
fuzztest/
├── CMakeLists.txt          # CMake configuration with FuzzTest integration
├── iot_parser.h            # C++ compatible header for IoT parser functions
├── iot_parser_fuzztest.cc  # Comprehensive fuzz tests and unit tests
├── build.sh                # Automated build script for both modes
├── README.md               # This documentation
├── fuzztest/               # FuzzTest framework (auto-cloned)
├── build/                  # Unit test mode build (created by build.sh)
└── build_fuzz/             # Fuzzing mode build (created by build.sh)
```

## Fuzz Test Coverage

### 🎯 **Targeted Vulnerabilities**

Our fuzz tests specifically target the **6 intentional bugs** in the IoT parser:

1. **Buffer Overflow** (`FuzzMqttTopicParsing`)
   - Targets `strcpy` without bounds checking in `parse_mqtt_topic()`
   - Uses arbitrary strings with size constraints
   - AddressSanitizer will catch buffer overflows immediately

2. **Integer Overflow** (`FuzzJsonExtraction`)  
   - Targets numeric parsing in `extract_json_value()`
   - Tests boundary conditions and large numbers
   - Verifies length calculations don't overflow

3. **Use-After-Free** (`FuzzHttpRequestParsing`)
   - Targets memory management in `parse_http_request()`
   - Tests various request formats and edge cases
   - Memory sanitizers catch use-after-free bugs

4. **Format String Vulnerability** (`FuzzHttpMethodPath`)
   - Targets `printf` usage in error handling
   - Includes format string characters (%s, %d, etc.) in input
   - Tests method/path combinations

### 🧪 **Comprehensive Test Suite**

#### **Unit Tests** (Basic Functionality)
- `ValidMqttParsing`: Test correct MQTT topic extraction
- `ValidJsonExtraction`: Test JSON key-value parsing  
- `ValidHttpParsing`: Test HTTP request parsing

#### **Property-Based Fuzz Tests**
- `FuzzMqttTopicParsing`: Arbitrary string input testing
- `FuzzJsonExtraction`: JSON + key combination testing
- `FuzzHttpRequestParsing`: HTTP request format testing
- `FuzzMqttStructured`: Structured MQTT input generation
- `FuzzJsonKeyValue`: JSON structure with prefix/suffix testing
- `FuzzHttpMethodPath`: HTTP method/path/body combinations
- `FuzzEdgeCases`: Boundary and edge case testing

## Quick Start

### 🚀 **One-Command Setup**

```bash
cd fuzztest/
./build.sh
```

This script will:
1. Clone FuzzTest framework automatically
2. Build in **Unit Test Mode** and run tests
3. Build in **Fuzzing Mode** for comprehensive fuzzing
4. Provide instructions for running individual fuzz tests

### 📋 **Prerequisites**

- **Linux** operating system
- **Clang** compiler (GCC not supported)
- **CMake** 3.19 or later
- **Git** for cloning FuzzTest

```bash
# Ubuntu/Debian
sudo apt install clang cmake git

# Verify installation
clang --version
cmake --version
```

## Usage Modes

### 1. **Unit Test Mode** (Default)

Runs both unit tests and fuzz tests for a **short duration** with **no sanitizers**:

```bash
cd fuzztest/build/
./iot_parser_fuzztest

# Expected output:
# [==========] Running X tests from 1 test suite.
# [----------] Global test environment set-up.
# [ RUN      ] IoTParserTest.ValidMqttParsing
# [       OK ] IoTParserTest.ValidMqttParsing (0 ms)
# [ RUN      ] IoTParserTest.FuzzMqttTopicParsing  
# [       OK ] IoTParserTest.FuzzMqttTopicParsing (13 ms)
# ...
```

### 2. **Fuzzing Mode** (Coverage-Guided)

Runs individual fuzz tests **indefinitely** with **full sanitizers** and **coverage instrumentation**:

```bash
cd fuzztest/build_fuzz/

# Run specific fuzz test
./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzMqttTopicParsing

# Expected output:
# [.] Sanitizer coverage enabled. Counter map size: 21290
# [*] Corpus size: 1 | Edges covered: 131 | Fuzzing time: 504.798us
# [*] Corpus size: 2 | Edges covered: 133 | Fuzzing time: 934.176us
# ...
```

### 3. **Time-Limited Fuzzing**

Run fuzzing for specific duration:

```bash
# Fuzz for 60 seconds
timeout 60 ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzMqttTopicParsing

# Fuzz for 5 minutes  
timeout 300 ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzJsonExtraction
```

## Finding Bugs

### 🐛 **Expected Results**

Since our IoT parser has **intentional vulnerabilities**, FuzzTest should find bugs quickly:

#### **Buffer Overflow Detection**
```bash
./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzMqttTopicParsing

# Expected crash output:
# =================================================================
# ==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
# WRITE of size 1 at 0x... thread T0
#     #0 0x... in parse_mqtt_topic iot_parser.c:35
#     #1 0x... in FuzzMqttTopicParsing iot_parser_fuzztest.cc:42
# ...
```

#### **Integer Overflow / Bounds Issues**
```bash
./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzJsonExtraction
# May trigger overflow in numeric parsing or string bounds violations
```

#### **Use-After-Free Detection**
```bash
./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzHttpRequestParsing
# AddressSanitizer will catch use-after-free in malloc/free handling
```

### 📊 **Coverage Analysis**

FuzzTest provides real-time coverage feedback:
- **Corpus size**: Number of interesting inputs found
- **Edges covered**: Code paths explored  
- **Fuzzing time**: Total execution time
- **Runs/secs**: Performance metrics

## Key Features

### 🎯 **Smart Input Generation**

FuzzTest uses **structured domains** instead of purely random mutations:

```cpp
// Constrained string generation
.WithDomains(fuzztest::Arbitrary<std::string>()
                 .WithMinSize(4)      // Minimum length
                 .WithMaxSize(512))   // Maximum length

// Multi-parameter fuzzing
.WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(500),
             fuzztest::Arbitrary<std::string>().WithMaxSize(30));

// Integer ranges
.WithDomains(fuzztest::InRange(0, 1000), 
             fuzztest::Arbitrary<std::string>().WithMaxSize(300));
```

### 🔍 **Property-Based Validation**

Each fuzz test validates **properties** that should always hold:

```cpp
// Example: Bounds checking
EXPECT_LE(strlen(msg.topic), MAX_TOPIC_SIZE - 1);
EXPECT_LE(strlen(msg.payload), MAX_BUFFER_SIZE - 1);

// Example: Non-negative results
EXPECT_GE(result, 0);

// Example: Null termination
if (result > 0) {
    EXPECT_LE(strlen(value), MAX_VALUE_SIZE - 1);
}
```

### 🛡️ **Automatic Sanitization**

- **AddressSanitizer**: Catches buffer overflows, use-after-free
- **UndefinedBehaviorSanitizer**: Catches integer overflows, null derefs
- **Coverage instrumentation**: Guides fuzzing to new code paths

## Comparison with Other Fuzzers

| Feature | **FuzzTest** | **Radamsa** | **boofuzz** |
|---------|-------------|-------------|-------------|
| **Type** | Property-based | Mutation-based | Protocol-aware |
| **Language** | C++ | Any (black-box) | Python |
| **Coverage** | Built-in guided | Manual analysis | Limited |
| **Integration** | GoogleTest native | External | Network-focused |
| **Input Structure** | Smart domains | Random mutations | Protocol models |
| **Learning** | ✅ Coverage-guided | ❌ Blind | ✅ Protocol-aware |
| **Bug Detection** | ✅ Immediate (sanitizers) | ✅ Post-analysis | ✅ Runtime |

## Advanced Usage

### 🔧 **Custom Domains**

Create specific input patterns:

```cpp
// MQTT-specific inputs
.WithDomains(fuzztest::OneOf(
    fuzztest::Just("mqtt/sensors/temp"),
    fuzztest::Just("mqtt/device/status"),
    fuzztest::Arbitrary<std::string>().WithMinSize(10)
));

// JSON-like structures  
.WithDomains(fuzztest::ConstructorOf<std::string>(
    fuzztest::PrintableAsciiString(),
    fuzztest::Just(":"),
    fuzztest::PrintableAsciiString()
));
```

### 📈 **Corpus Management**

FuzzTest automatically maintains a **corpus** of interesting inputs:
- Inputs that increase coverage
- Inputs that trigger new code paths
- Minimized test cases for reproduction

### 🐞 **Reproducing Crashes**

When a crash is found:
1. FuzzTest reports the exact input that caused the crash
2. You can reproduce it by running the unit test with that input
3. AddressSanitizer provides detailed stack traces

## Next Steps

1. **Run the fuzz tests** and observe bug discovery
2. **Analyze crash reports** to understand vulnerabilities  
3. **Extend fuzz tests** for additional functions or protocols
4. **Integrate with CI/CD** for continuous security testing
5. **Experiment with custom domains** for protocol-specific testing

## References

- [FuzzTest Documentation](https://github.com/google/fuzztest)
- [CMake Integration Guide](https://github.com/google/fuzztest/blob/main/doc/quickstart-cmake.md)
- [Property-Based Testing Concepts](https://github.com/google/fuzztest/blob/main/doc/tutorial.md) 