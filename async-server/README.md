# Async Server Examples: pthread vs C++20 Coroutines

This project demonstrates two different approaches to handling multiple concurrent client connections in a server application running on ESP32:

1. **Traditional pthread approach** (C)
2. **Modern C++20 coroutines approach**

Both implementations serve the same purpose: handling multiple HTTP clients simultaneously while serving an HTML file in chunks with simulated processing delays.

## Purpose

The goal is to showcase the differences between:
- **Thread-based concurrency** (pthread) - traditional approach with explicit thread management
- **Coroutine-based concurrency** (C++20) - modern approach with implicit async/await semantics

This comparison helps understand the trade-offs between these concurrency models for the same use case on embedded systems like ESP32.

## Implementation Details

### Common Features
Both servers implement:
- HTTP server on port 8080
- Serving `index.html` file (embedded in binary)
- Chunked file transfer (100 bytes per chunk)
- Simulated processing delays (1 second for pthread, 200ms for coroutines)
- Multiple client handling
- FreeRTOS task statistics monitoring

### 1. pthread Server (`async_server_pthread.c`)

**Language**: C  
**Concurrency Model**: Multi-threading with pthread  
**Key Characteristics**:
- Creates a new thread for each client connection
- Each thread handles one client completely
- Uses `pthread_create()` and `pthread_detach()`
- Explicit thread management
- Traditional blocking I/O operations

**Code Structure**:
```c
void *handle_client(void *arg) {
    // Handle single client in dedicated thread
    // Send HTTP headers
    // Send file in chunks with sleep(1)
    // Close connection
}

// Main loop creates threads
pthread_t tid;
pthread_create(&tid, NULL, handle_client, cargs);
pthread_detach(tid);
```

**Pros**:
- Simple to understand
- True parallelism
- Standard approach

**Cons**:
- Higher memory overhead per connection
- Context switching overhead
- More complex resource management

### 2. Coroutine Server (`async_server_coroutines.cpp`)

**Language**: C++20  
**Concurrency Model**: Coroutines with async/await  
**Key Characteristics**:
- Uses C++20 coroutines for async operations
- Single-threaded with cooperative multitasking
- Implicit async/await semantics
- Non-blocking I/O simulation

**Code Structure**:
```cpp
Task handle_client(int client_sock, const char* file_data, size_t file_size) {
    // Send HTTP headers
    co_await AsyncSend{client_sock, headers.str().data(), headers.str().size()};
    
    // Send file in chunks
    for (size_t offset = 0; offset < file_size; offset += chunk_size) {
        co_await AsyncSend{client_sock, file_data + offset, chunk};
        co_await Sleep{std::chrono::milliseconds(200)};
    }
}
```

**Pros**:
- Lower memory overhead
- No context switching
- More readable async code
- Better resource utilization

**Cons**:
- Requires C++20 compiler
- Single-threaded (no true parallelism)
- More complex coroutine infrastructure

## Building and Running

### Prerequisites
- ESP-IDF development environment
- ESP32 development board
- Python 3.6+ for testing

### Building for ESP32

**pthread version**:
```bash
cd async-server/server
idf.py build
idf.py flash monitor
```

**Coroutine version**:
```bash
cd async-server/server
# Edit CMakeLists.txt to use async_server_coroutines.cpp
idf.py build
idf.py flash monitor
```

Both servers will listen on port 8080 and connect to your WiFi network.

## Testing

### Python Test Script

Use the included `test_server.py` script to test both server implementations:

```bash
# Test with 5 concurrent clients
python3 test_server.py 192.168.1.100 5

# Test with 10 clients but limit to 3 concurrent connections
python3 test_server.py 192.168.1.100 10 --max-workers 3

# Test on different port
python3 test_server.py 192.168.1.100 8 --port 8080
```

The test script provides:
- Concurrent client testing
- Performance metrics (response time, throughput)
- Success/failure analysis
- Comprehensive reporting

### Manual Testing

1. Start either server on ESP32
2. Open multiple browser tabs to `http://[ESP32_IP]:8080`
3. Observe the server console output showing concurrent client handling
4. Monitor FreeRTOS task statistics printed every 500ms

## Performance Comparison

### FreeRTOS Task Statistics

The server includes built-in FreeRTOS monitoring that prints statistics every 500ms, showing:

**pthread version**:
```
=== FreeRTOS Task Statistics ===
Total tasks: 19
Task Name            State    Prio     Stack    CPU%%   
------------------------------------------------------------
Tmr Svc              RUN      1        20       10%     
IDLE1                RDY      0        1036     95%     
IDLE0                RDY      0        912      86%     
pthread              BLK      5        1048     <1%     
pthread              BLK      5        1044     <1%     
pthread              BLK      5        1048     <1%     
pthread              BLK      5        1048     <1%     
pthread              BLK      5        1012     <1%     
pthread              BLK      5        1040     <1%     
main                 BLK      1        1220     <1%     
pthread              BLK      5        1008     <1%     
pthread              BLK      5        1020     <1%     
---
Free heap: 178816 bytes, Min free heap: 167692 bytes
[Thread 1073512348] Sent 100 bytes
[Thread 1073527232] Sent 100 bytes
[Thread 1073537060] Sent 100 bytes
[Thread 1073542920] Sent 100 bytes
[Thread 1073541272] Sent 100 bytes
```

**Coroutines version**:
```
=== FreeRTOS Task Statistics ===
Total tasks: 12
Task Name            State    Prio     Stack    CPU%%   
------------------------------------------------------------
Tmr Svc              RUN      1        124      6%      
IDLE1                RDY      0        1028     98%     
IDLE0                RDY      0        924      85%     
tiT                  BLK      18       1420     1%      
main                 BLK      1        1220     <1%     
pthread              BLK      5        1520     1%      
============================================================
Free heap: 189524 bytes, Min free heap: 176276 bytes
[Coroutine 55] Sent 100 bytes
[Coroutine 56] Sent 100 bytes
[Coroutine 57] Sent 100 bytes
[Coroutine 58] Sent 100 bytes
[Coroutine 59] Sent 100 bytes
[Coroutine 60] Sent 100 bytes
[Coroutine 61] Sent 100 bytes
[Coroutine 62] Sent 100 bytes
[Coroutine 55] Sent 100 bytes
[Coroutine 56] Sent 100 bytes
[Coroutine 57] Sent 100 bytes
```

### Real-World Performance Test Results

Testing with 8 concurrent clients shows the following performance characteristics:

```
=== Starting concurrent test with 8 clients ===
Server: 192.168.36:8080
Max concurrent connections: 8
----------------------------------------------------------------------
Client   0:  11.89s,   5943 bytes,    499.7 B/s, ✓
Client   1:  11.90s,   5943 bytes,    499.5 B/s, ✓
Client   2:  11.92s,   5943 bytes,    498.7 B/s, ✓
Client   3:  11.92s,   5943 bytes,    498.7 B/s, ✓
Client   4:  11.92s,   5943 bytes,    498.7 B/s, ✓
Client   6:  12.82s,   5943 bytes,    463.7 B/s, ✓
Client   7:  12.82s,   5943 bytes,    463.5 B/s, ✓
Client   5:  12.83s,   5943 bytes,    463.3 B/s, ✓

======================================================================
TEST RESULTS SUMMARY
======================================================================
Total clients: 8
Successful: 8
Failed: 0
Total test duration: 12.83 seconds
```

### Key Observations

1. **Task Count**: pthread version creates more tasks (19 vs 12) due to per-client threading
2. **Memory Usage**: pthread version shows lower free heap (178,816 vs 189,524 bytes)
3. **CPU Utilization**: Both show similar idle CPU usage patterns
4. **Stack Usage**: pthread tasks use ~1KB stack each, coroutines use shared stack
5. **Concurrent Handling**: Successfully handles 8 simultaneous clients with consistent performance

## Key Differences Summary

| Aspect | pthread (C) | Coroutines (C++20) |
|--------|-------------|-------------------|
| **Concurrency Model** | Multi-threading | Cooperative multitasking |
| **Memory per Client** | Higher (thread stack) | Lower (coroutine frame) |
| **True Parallelism** | Yes | No (single-threaded) |
| **Context Switching** | OS-level | Application-level |
| **Resource Management** | Explicit | Implicit |
| **Code Readability** | Traditional | Modern async/await |
| **FreeRTOS Tasks** | 19 (more overhead) | 12 (more efficient) |
| **Heap Usage** | Higher memory footprint | Lower memory footprint |

## Use Cases

**Choose pthread when**:
- You need true parallelism on ESP32
- Working in C environment
- Simple, well-understood threading model
- Maximum performance is critical

**Choose coroutines when**:
- Working in modern C++ on ESP32
- Memory efficiency is important
- You prefer async/await semantics
- Single-threaded performance is sufficient

## Files

- `server/main/server.c` - Main server application with FreeRTOS monitoring
- `server/main/async_server_pthread.c` - pthread-based server implementation
- `server/main/async_server_coroutines.cpp` - C++20 coroutine-based server implementation
- `test_server.py` - Python testing script for performance analysis
- `README.md` - This documentation

## Future Enhancements

Potential improvements could include:
- Performance benchmarking with larger client loads
- Memory usage analysis and optimization
- Error handling improvements
- HTTP request parsing
- Support for different file types
- Load testing with multiple concurrent clients
- Power consumption analysis on ESP32
