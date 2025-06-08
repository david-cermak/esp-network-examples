# Radamsa Fuzzing of IoT Network Message Parser

This directory contains fuzzing experiments using [Radamsa](https://gitlab.com/akihe/radamsa), a test case generator for robustness testing (fuzzing). Radamsa is a mutation-based fuzzer that takes sample inputs and creates variations by flipping bits, duplicating data, inserting garbage, and other mutations.

## Overview

**Target Program**: `../sample/iot_parser.c` - IoT Network Message Parser  
**Fuzzing Method**: Mutation-based black-box fuzzing  
**Tool**: Radamsa with custom bash wrapper script  
**Detection**: AddressSanitizer (ASAN) + UndefinedBehaviorSanitizer (UBSAN)

## Quick Start

```bash
# Make sure the target is built with sanitizers
cd ../sample && rm -rf build && mkdir build && cd build
cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON .. && make

# Run fuzzing (from radamsa directory)
cd ../radamsa
./run_fuzzer.sh -n 1000    # 1000 iterations
```

## Files

- `run_fuzzer.sh` - Main fuzzing script with comprehensive logging
- `sample_inputs.txt` - Seed inputs (realistic IoT messages)
- `crashes/` - Inputs that caused program crashes (signals)
- `timeouts/` - Inputs that caused program hangs
- `errors/` - Inputs that caused other errors (non-zero exits)
- `fuzzing.log` - Detailed log of all failures with timestamps

## Sample Seed Inputs

The fuzzer uses these realistic IoT protocol messages as seeds:

```
mqtt/home/temperature {"device_id":"sensor01","temperature":23.5,"status":"active"}
mqtt/device/led {"action":"toggle","brightness":80,"color":"red"}
GET /api/sensors {"action":"read","sensor":"temperature"}
POST /device/control {"action":"set","value":"on","device":"fan"}
```

## Usage Examples

```bash
# Basic fuzzing
./run_fuzzer.sh

# Custom number of runs
./run_fuzzer.sh -n 5000

# With custom timeout
./run_fuzzer.sh -n 1000 -t 10

# Help
./run_fuzzer.sh -h
```

## Results Analysis

### Example Bug Found

**AddressSanitizer Report Analysis:**
```
==65775==ERROR: AddressSanitizer: stack-buffer-overflow
WRITE of size 490 at 0x7ffc1c7ab4b0
#1 0x55bb606fc841 in parse_mqtt_topic .../iot_parser.c:37
```

**What this means:**
- **Bug**: Stack buffer overflow in `parse_mqtt_topic()` function
- **Root Cause**: `strcpy(msg->payload, space_pos + 1)` without bounds checking
- **Trigger**: 490-byte payload written to 256-byte buffer
- **Overflow**: 234 bytes beyond buffer boundary
- **Impact**: Stack corruption, potential code execution

**Why Radamsa found it:**
- Generated an MQTT message with unusually long payload
- Mutation exceeded expected input size limits
- No input validation in the parser
- AddressSanitizer detected the overflow immediately

### Vulnerability Categories Found

1. **Buffer Overflows** - Long payloads/topics overflow fixed-size buffers
2. **Format String Bugs** - Malformed error messages can exploit printf
3. **Integer Overflows** - Large numeric values in JSON parsing
4. **Use-After-Free** - Complex parsing sequences trigger memory bugs
5. **Null Pointer Dereferences** - Malformed inputs bypass null checks

## Key Takeaways

### ✅ **Radamsa Effectiveness**
- **Black-box approach**: No source code knowledge required
- **Mutation diversity**: Creates unexpected input variations
- **Quick results**: Found buffer overflow in < 1000 iterations
- **Easy setup**: Simple command-line tool, no complex configuration

### ✅ **AddressSanitizer Value**
- **Immediate detection**: Catches memory bugs instantly
- **Detailed reports**: Shows exact location and stack trace
- **No false positives**: Every report indicates a real bug
- **Production ready**: Minimal performance overhead

### ✅ **Fuzzing Best Practices Demonstrated**
- **Seed inputs**: Start with valid, realistic examples
- **Comprehensive logging**: Save all failure-inducing inputs
- **Sanitizer integration**: Catch subtle bugs that don't crash
- **Categorized results**: Separate crashes, timeouts, and errors
- **Reproducible**: Save exact inputs for debugging

## Comparison with Other Fuzzers

| Aspect | Radamsa | AFL++ | libFuzzer |
|--------|---------|-------|-----------|
| **Type** | Mutation-based | Coverage-guided | Coverage-guided |
| **Setup** | Very easy | Moderate | Complex |
| **Source needed** | No | Instrumentation | Instrumentation |
| **Speed** | Fast | Very fast | Very fast |
| **Intelligence** | Low | High | High |
| **Best for** | Quick testing | Deep exploration | Continuous testing |

## Limitations

- **No coverage feedback**: Doesn't learn which paths are interesting
- **Random mutations**: May miss deep bugs requiring specific sequences
- **No state awareness**: Doesn't understand protocol semantics
- **Limited depth**: Surface-level bugs found faster than complex ones

## Next Steps

1. **Try AFL++**: Compare coverage-guided vs mutation-based results
2. **Protocol-aware fuzzing**: Use tools like boofuzz for stateful testing
3. **Custom mutators**: Create IoT-specific mutation strategies
4. **Combine approaches**: Use radamsa for initial discovery, AFL++ for deep exploration

## References

- [Radamsa Project](https://gitlab.com/akihe/radamsa)
- [AddressSanitizer Documentation](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [The Fuzzing Book](https://www.fuzzingbook.org/)
- [Fuzzing: Art, Science, and Engineering](https://arxiv.org/abs/1812.00140)

## Commands Reference

```bash
# Install radamsa (Ubuntu/Debian)
sudo apt-get install radamsa

# Build target with sanitizers
cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..

# Run single test with saved input
cat crashes/crash_1749409476_217.txt | ../sample/build/iot_parser

# Analyze specific failure type
ls crashes/     # All crashing inputs
ls timeouts/    # All timeout-causing inputs  
ls errors/      # All error-causing inputs

# Check detailed logs
cat fuzzing.log | grep "Crash"
``` 