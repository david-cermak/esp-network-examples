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

// Awaitable sleep
struct Sleep {
    std::chrono::milliseconds duration;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const {
        std::thread([=] {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }
    void await_resume() const noexcept {}
};

// Awaitable send wrapper
struct AsyncSend {
    int sock;
    std::string data;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        std::thread([=] {
            send(sock, data.data(), data.size(), 0);
            h.resume();
        }).detach();
    }
    void await_resume() const noexcept {}
};

// === Coroutine client handler ===
Task handle_client(int client_sock, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << filename << "\n";
        close(client_sock);
        co_return;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create HTTP headers
    std::stringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Length: " << file_size << "\r\n";
    headers << "Content-Type: text/html\r\n";
    headers << "Connection: close\r\n";
    headers << "\r\n";

    // Send headers
    co_await AsyncSend{client_sock, headers.str()};

    // Send file content in chunks
    std::vector<char> buffer(100);
    while (file.good()) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            std::string chunk(buffer.data(), bytes_read);
            co_await AsyncSend{client_sock, chunk};
            co_await Sleep{std::chrono::milliseconds(200)};
            std::cout << "[Coroutine " << client_sock << "] Sent " << bytes_read << " bytes\n";
        }
    }

    close(client_sock);
    std::cout << "Finished client " << client_sock << "\n";
}

// === Main server loop ===
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "Coroutine server listening on port 8080...\n";

    while (true) {
        int client_sock = accept(server_fd, nullptr, nullptr);
        if (client_sock >= 0) {
            std::cout << "New client: " << client_sock << "\n";
            handle_client(client_sock, "index.html"); // fire-and-forget coroutine
        }
    }
}
