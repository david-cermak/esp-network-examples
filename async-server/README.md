# Async Server Examples: pthread vs C++20 Coroutines

This project demonstrates two different approaches to handling multiple concurrent client connections in a server application:

1. **Traditional pthread approach** (C)
2. **Modern C++20 coroutines approach**

Both implementations serve the same purpose: handling multiple HTTP clients simultaneously while serving an HTML file in chunks with simulated processing delays.

## Purpose

The goal is to showcase the differences between:
- **Thread-based concurrency** (pthread) - traditional approach with explicit thread management
- **Coroutine-based concurrency** (C++20) - modern approach with implicit async/await semantics

This comparison helps understand the trade-offs between these concurrency models for the same use case.

## Implementation Details

### Common Features
Both servers implement:
- HTTP server on port 8080
- Serving `index.html` file
- Chunked file transfer (100 bytes per chunk)
- Simulated processing delays (1 second for pthread, 200ms for coroutines)
- Multiple client handling

### 1. pthread Server (`server.c`)

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

### 2. Coroutine Server (`coroutines.cpp`)

**Language**: C++20  
**Concurrency Model**: Coroutines with async/await  
**Key Characteristics**:
- Uses C++20 coroutines for async operations
- Single-threaded with cooperative multitasking
- Implicit async/await semantics
- Non-blocking I/O simulation

**Code Structure**:
```cpp
Task handle_client(int client_sock, const std::string &filename) {
    // Send HTTP headers
    co_await AsyncSend{client_sock, headers.str()};
    
    // Send file in chunks
    while (file.good()) {
        co_await AsyncSend{client_sock, chunk};
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
- GCC with C++20 support for coroutines
- Standard C compiler for pthread version

### Compilation

**pthread version**:
```bash
gcc -o server server.c -lpthread
```

**Coroutine version**:
```bash
g++ -std=c++20 -o coroutine_server coroutines.cpp
```

### Running

**pthread server**:
```bash
./server
```

**Coroutine server**:
```bash
./coroutine_server
```

Both servers will listen on port 8080.

## Testing

1. Start either server
2. Open multiple browser tabs to `http://localhost:8080`
3. Observe the server console output showing concurrent client handling
4. Notice the different timing patterns:
   - pthread: 1-second delays between chunks
   - coroutines: 200ms delays between chunks

## Key Differences Summary

| Aspect | pthread (C) | Coroutines (C++20) |
|--------|-------------|-------------------|
| **Concurrency Model** | Multi-threading | Cooperative multitasking |
| **Memory per Client** | Higher (thread stack) | Lower (coroutine frame) |
| **True Parallelism** | Yes | No (single-threaded) |
| **Context Switching** | OS-level | Application-level |
| **Resource Management** | Explicit | Implicit |
| **Code Readability** | Traditional | Modern async/await |
| **Compiler Support** | Standard C | C++20 required |

## Use Cases

**Choose pthread when**:
- You need true parallelism
- Working in C environment
- Simple, well-understood threading model
- Maximum performance is critical

**Choose coroutines when**:
- Working in modern C++
- Memory efficiency is important
- You prefer async/await semantics
- Single-threaded performance is sufficient

## Files

- `server.c` - pthread-based server implementation
- `coroutines.cpp` - C++20 coroutine-based server implementation
- `index.html` - Test HTML page served by both servers
- `README.md` - This documentation

## Future Enhancements

Potential improvements could include:
- Performance benchmarking
- Memory usage analysis
- Error handling improvements
- HTTP request parsing
- Support for different file types
- Load testing with multiple concurrent clients
