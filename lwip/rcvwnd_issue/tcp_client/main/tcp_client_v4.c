/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#include "addr_from_stdin.h"
#endif

// Default configuration for testing
#ifndef CONFIG_EXAMPLE_IPV4_ADDR
#define CONFIG_EXAMPLE_IPV4_ADDR "127.0.0.1"
#endif

#ifndef CONFIG_EXAMPLE_PORT
#define CONFIG_EXAMPLE_PORT 3333
#endif

#if defined(CONFIG_EXAMPLE_IPV4) || !defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#define HOST_IP_ADDR "192.168.0.32"
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#define HOST_IP_ADDR ""
#endif

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "example";
static const char *payload = "GET /test HTTP/1.1\r\nHost: test\r\nConnection: close\r\n\r\n";
static const int EXPECTED_RESPONSE_SIZE = 1500;  // Expect ~1380 bytes + headers

void tcp_client(void)
{
    char rx_buffer[1500];  // Increased buffer to handle 1380+ bytes
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
        struct sockaddr_in dest_addr;
        struct sockaddr_storage dest_addr_storage = { 0 };
        
#if defined(CONFIG_EXAMPLE_IPV4) || !defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr_storage));
#endif

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        // Set small receive window size to force tcplen > rcv_wnd condition
        int rcvbuf = 512;  // Set receive buffer to 512 bytes (to match our testing)
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
            ESP_LOGE(TAG, "Failed to set SO_RCVBUF: errno %d", errno);
        }
        ESP_LOGI(TAG, "Set receive buffer to %d bytes", rcvbuf);
        
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        // Send GET request
        ESP_LOGI(TAG, "Sending GET request...");
        int send_result = send(sock, payload, strlen(payload), 0);
        if (send_result < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Sent %d bytes: %s", send_result, payload);

        // Receive response - handle potentially large packets
        int total_received = 0;
        int receive_attempts = 0;
        const int max_attempts = 10;
        
        while (total_received < EXPECTED_RESPONSE_SIZE && receive_attempts < max_attempts) {
            int remaining_buffer = sizeof(rx_buffer) - total_received - 1; // -1 for null terminator
            if (remaining_buffer <= 0) {
                ESP_LOGW(TAG, "Buffer full, cannot receive more data");
                break;
            }
            
            int len = recv(sock, rx_buffer + total_received, remaining_buffer, 0);
            receive_attempts++;
            
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGW(TAG, "Connection closed by server");
                break;
            } else {
                total_received += len;
                ESP_LOGI(TAG, "Received %d bytes (attempt %d), total: %d/%d bytes", 
                        len, receive_attempts, total_received, EXPECTED_RESPONSE_SIZE);
                
                // Check if we received the expected amount
                if (total_received >= EXPECTED_RESPONSE_SIZE) {
                    ESP_LOGI(TAG, "Successfully received expected %d bytes", EXPECTED_RESPONSE_SIZE);
                    break;
                }
            }
        }

        if (total_received > 0) {
            rx_buffer[total_received] = 0; // Null-terminate
            ESP_LOGI(TAG, "Total received: %d bytes from %s", total_received, host_ip);
            ESP_LOGI(TAG, "Response preview (first 100 chars): %.100s%s", 
                    rx_buffer, total_received > 100 ? "..." : "");
                    
            // Log packet reception pattern for analysis
            if (receive_attempts > 1) {
                ESP_LOGW(TAG, "Data received in %d separate packets - potential partial ACK behavior", receive_attempts);
            } else {
                ESP_LOGI(TAG, "Data received in single packet - full packet ACK behavior");
            }
        }

        if (sock != -1) {
            ESP_LOGI(TAG, "Closing socket and waiting before next connection...");
            shutdown(sock, 0);
            close(sock);
        }
        
        // Wait 5 seconds before next connection attempt
        sleep(5);
    }
}
