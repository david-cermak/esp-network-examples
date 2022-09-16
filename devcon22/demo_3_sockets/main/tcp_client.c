/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_eventfd.h"


#if defined(CONFIG_EXAMPLE_IPV4)
//#define HOST_IP_ADDR "192.168.1.108"
#define HOST_IP_ADDR "1.2.3.4"
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR ""
#endif

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";
static int disconnect_fd;

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
            ESP_LOGI(TAG, "Ethernet Link Up");
            ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "Ethernet Link Down");
            uint64_t signal = 1;
            ssize_t val = write(disconnect_fd, &signal, sizeof(signal));
            assert(val == sizeof(signal));
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;
        default:
            break;
    }
}

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = { 0 };
        inet6_aton(host_ip, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_storage dest_addr = { 0 };
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);
        int flags;
        if ((flags = fcntl(sock, F_GETFL, NULL)) < 0) {
            ESP_LOGE(TAG, "[sock=%d] get file flags error: %s", sock, strerror(errno));
            break;
        }

        flags |= O_NONBLOCK;
        if (fcntl(sock, F_SETFL, flags) < 0) {
            ESP_LOGE(TAG, "[sock=%d] set blocking/nonblocking error: %s", sock, strerror(errno));
            break;
        }
        ESP_LOGD(TAG, "[sock=%d] Connecting to server", sock);
        if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6)) < 0) {
            if (errno == EINPROGRESS) {
                fd_set fdset;
                struct timeval tv = { .tv_usec = 0, .tv_sec = 10 }; // Default connection timeout is 10 s

                FD_ZERO(&fdset);
                FD_SET(sock, &fdset);
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(disconnect_fd, &readfds);

                int maxFd = disconnect_fd > sock ? disconnect_fd : sock;

                int res = select(maxFd+1, &readfds, &fdset, NULL, &tv);
                printf("[sock=%d] select() returned %d\n", sock, res);
                if (res < 0) {
                    ESP_LOGE(TAG, "[sock=%d] select() error: %s", sock, strerror(errno));
                    break;
                }
                else if (res == 0) {
                    ESP_LOGE(TAG, "[sock=%d] select() timeout", sock);
                    break;
                } else {
                    if (FD_ISSET(disconnect_fd, &readfds)) {
                        ESP_LOGE(TAG, "[sock=%d] Disconnected! exiting...", sock);
                        lwip_close(sock);
                        ESP_LOGI(TAG, "After close");
                        continue;
                    }
                    int sockerr;
                    socklen_t len = (socklen_t)sizeof(int);

                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                        ESP_LOGE(TAG, "[sock=%d] getsockopt() error: %s", sock, strerror(errno));
                        break;
                    }
                    else if (sockerr) {
                        ESP_LOGE(TAG, "[sock=%d] delayed connect error: %s", sock, strerror(sockerr));
                        break;
                    }
                }
            } else {
                ESP_LOGE(TAG, "[sock=%d] connect() error: %s", sock, strerror(errno));
                break;
            }
        }
        ESP_LOGI(TAG, "Successfully connected");
        struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = 100;
        int ret = setsockopt(sock, SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);
        ESP_LOGI(TAG, "Socket option SO_LINGER returned %d", ret);
        vTaskDelay(pdMS_TO_TICKS(100));
//        usleep(500000);
        ESP_LOGE(TAG, "Sending...");
        int err = send(sock, payload, 9, 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
//        usleep(100);
        ESP_LOGE(TAG, "Before close!");
        lwip_close(sock);
        ESP_LOGI(TAG, "After close");
        continue;

        err = send(sock, payload, strlen(payload), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }
//            usleep(100);
//            ESP_LOGE(TAG, "Before close!");
//            lwip_close(sock);
//            ESP_LOGI(TAG, "After close");
//            continue;

            int len = recv(sock, rx_buffer, 5, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                if (len == 5) {
                    int ret;
                    ESP_LOGI(TAG, "Before close!");
//                    usleep(1000);
                    ret = send(sock, payload, strlen(payload), 0);
                    ESP_LOGI(TAG, "Socket sendreturned %d", ret);
//                     ret = setsockopt(sock, SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);
//                    ESP_LOGI(TAG, "Socket option SO_LINGER returned %d", ret);

                    lwip_close(sock);
                    ESP_LOGI(TAG, "After close");
                    continue;
                }

                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(example_connect());
    esp_vfs_eventfd_config_t config = ESP_VFS_EVENTD_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&config));

    disconnect_fd = eventfd(0, EFD_SUPPORT_ISR);
    assert(disconnect_fd > 0);

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
