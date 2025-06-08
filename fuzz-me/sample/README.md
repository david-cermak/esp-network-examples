# IoT Network Message Parser

A simple C program that parses IoT network messages (MQTT and HTTP) with JSON-like payloads. This program is designed for fuzzing experiments and contains several subtle bugs that can be discovered using various fuzzing tools.

## Building

### Basic build:
```bash
mkdir build && cd build
cmake ..
make
```

### Build with AddressSanitizer:
```bash
mkdir build && cd build
cmake -DENABLE_ASAN=ON ..
make
```

### Build with UndefinedBehaviorSanitizer:
```bash
mkdir build && cd build
cmake -DENABLE_UBSAN=ON ..
make
```

### Build with both sanitizers:
```bash
mkdir build && cd build
cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
make
```

## Usage

The program reads from stdin and expects one of these message formats:

### MQTT Messages:
```
mqtt/home/temperature {"device_id":"sensor01","temperature":23.5,"status":"active"}
mqtt/device/led {"action":"toggle","brightness":80}
```

### HTTP Messages:
```
GET /api/sensors {"action":"read","sensor":"temperature"}
POST /device/control {"action":"set","value":"on"}
```

## Example Session:
```bash
$ ./iot_parser
IoT Network Message Parser
Supported formats:
  MQTT: mqtt/topic/path {"key":"value"}
  HTTP: GET /path {"key":"value"}
  HTTP: POST /path {"key":"value"}
Enter message: mqtt/home/temp {"device_id":"esp32","temperature":25}

=== MQTT Message Analysis ===
Topic: /home/temp
Payload: {"device_id":"esp32","temperature":25}
Device ID: esp32
Temperature: 25
```

## Fuzzing Targets

This program contains several intentional subtle bugs perfect for fuzzing:

1. **Buffer overflows** in string copying operations
2. **Off-by-one errors** in string manipulation  
3. **Integer overflows** in numeric parsing
4. **Use-after-free** vulnerabilities in memory management
5. **Format string vulnerabilities** in error handling
6. **Null pointer dereferences** with malformed input

These bugs are designed to be discoverable by fuzzing tools like AFL++, libFuzzer, or radamsa when combined with AddressSanitizer and UndefinedBehaviorSanitizer. 