#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <coroutine>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

// === Event Loop for Coroutines ===
class EventLoop {
private:
    struct ScheduledTask {
        std::chrono::steady_clock::time_point when;
        std::function<void()> task;
        
        bool operator>(const ScheduledTask& other) const {
            return when > other.when;
        }
    };

    std::thread worker_thread;
    std::queue<std::function<void()>> immediate_tasks;
    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, std::greater<ScheduledTask>> scheduled_tasks;
    std::mutex tasks_mutex;
    std::condition_variable tasks_cv;
    bool running = false;

public:
    void start() {
        running = true;
        worker_thread = std::thread([this]() {
            while (running) {
                std::function<void()> task;
                bool has_work = false;
                
                {
                    std::unique_lock<std::mutex> lock(tasks_mutex);
                    
                    // Check for immediate tasks first
                    if (!immediate_tasks.empty()) {
                        task = std::move(immediate_tasks.front());
                        immediate_tasks.pop();
                        has_work = true;
                    }
                    // Check for scheduled tasks that are ready
                    else if (!scheduled_tasks.empty()) {
                        auto now = std::chrono::steady_clock::now();
                        if (scheduled_tasks.top().when <= now) {
                            task = std::move(scheduled_tasks.top().task);
                            scheduled_tasks.pop();
                            has_work = true;
                        }
                    }
                    
                    // If no immediate work, wait for either immediate tasks or scheduled tasks
                    if (!has_work) {
                        if (scheduled_tasks.empty()) {
                            tasks_cv.wait(lock, [this]() { return !immediate_tasks.empty() || !running; });
                        } else {
                            auto next_task_time = scheduled_tasks.top().when;
                            tasks_cv.wait_until(lock, next_task_time, [this]() { 
                                return !immediate_tasks.empty() || !running; 
                            });
                        }
                        continue; // Re-check after wait
                    }
                }
                
                if (task) task();
            }
        });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            running = false;
        }
        tasks_cv.notify_all();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    void post(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            immediate_tasks.push(std::move(task));
        }
        tasks_cv.notify_one();
    }

    void post_delayed(std::function<void()> task, std::chrono::milliseconds delay) {
        auto when = std::chrono::steady_clock::now() + delay;
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            scheduled_tasks.push({when, std::move(task)});
        }
        tasks_cv.notify_one();
    }
};

// Global event loop instance
static EventLoop event_loop;

// === Coroutine primitives ===
struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

// Awaitable sleep - now uses event loop instead of creating threads
struct Sleep {
    std::chrono::milliseconds duration;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const {
        event_loop.post_delayed([h]() { h.resume(); }, duration);
    }
    void await_resume() const noexcept {}
};

// Awaitable send wrapper - now uses event loop instead of creating threads
struct AsyncSend {
    int sock;
    const char* data;
    size_t size;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        event_loop.post([this, h]() {
            send(sock, data, size, 0);
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

// === Coroutine client handler ===
Task handle_client(int client_sock, const char* file_data, size_t file_size) {
    // Create HTTP headers
    std::stringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Length: " << file_size << "\r\n";
    headers << "Content-Type: text/html\r\n";
    headers << "Connection: close\r\n";
    headers << "\r\n";

    // Send headers
    co_await AsyncSend{client_sock, headers.str().data(), headers.str().size()};

    // Send file content in chunks
    const size_t chunk_size = 100;
    for (size_t offset = 0; offset < file_size; offset += chunk_size) {
        size_t chunk = (file_size - offset > chunk_size) ? chunk_size : (file_size - offset);
        co_await AsyncSend{client_sock, file_data + offset, chunk};
        co_await Sleep{std::chrono::milliseconds(200)};
        std::cout << "[Coroutine " << client_sock << "] Sent " << chunk << " bytes\n";
    }

    close(client_sock);
    std::cout << "Finished client " << client_sock << "\n";
}

// === Main server loop ===
// int main() {
extern "C" void async_server(void) 
{
    // Start the event loop
    event_loop.start();

    // Read file into memory
    const char* filename = "index.html";
    char* file_data;
    size_t file_size;
    
#if 0
    // Read file from filesystem (Linux)
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << filename << "\n";
        return;
    }
    
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    file_data = new char[file_size];
    file.read(file_data, file_size);
    file.close();
#else
    // Use embedded data (ESP32)
    extern const uint8_t index_html[] asm("_binary_index_html_start");
    extern const uint8_t index_html_end[] asm("_binary_index_html_end");
    file_size = index_html_end - index_html;
    file_data = new char[file_size];
    std::memcpy(file_data, index_html, file_size);
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    std::cout << "Coroutine server listening on port 8080...\n";

    while (true) {
        int client_sock = accept(server_fd, nullptr, nullptr);
        if (client_sock >= 0) {
            std::cout << "New client: " << client_sock << "\n";
            handle_client(client_sock, file_data, file_size); // fire-and-forget coroutine
        }
    }
    
    delete[] file_data;
    event_loop.stop();
}
