#ifdef ESP_LWIP
#include "lwip/opt.h"
#endif

#if LWIP_SOCKET
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwipcfg.h"
#include "default_netif.h"
#include "lwip/netdb.h"
#else
#define sys_msleep(ms) usleep(1000*ms)
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <netdb.h>
#include <fcntl.h>
// #include <select.h>
#endif

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INVALID_SOCK (-1)


#define ESP_LOGV(tag, fmt, ...)
// #define ESP_LOGI(tag, fmt, ...) do { printf("(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
// #define ESP_LOGE(tag, fmt, ...) do { printf("ERROR(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
// #define ESP_LOGW(tag, fmt, ...) do { printf("WARN(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGE(tag, fmt, ...)
#define ESP_LOGW(tag, fmt, ...)
// #define HOST_IP_ADDR "192.168.1.1"
#define HOST_IP_ADDR "0.0.0.0"
#define PORT "3333"
#define CONFIG_LWIP_MAX_SOCKETS 1000


#ifndef ESP_LWIP

static char* inet_ntoa_r(struct in_addr addr, char* ptr, size_t size)
{
    char * res = inet_ntoa(addr);
    if (res && strlen(res) < size) {
        strcpy(ptr, res);
    }
    return res;
}
#endif
/**
 * @brief Utility to log socket errors
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket number
 * @param[in] err Socket errno
 * @param[in] message Message to print
 */
static void log_socket_error(const char *tag, const int sock, const int err, const char *message)
{
    (void)tag;
    (void)sock;
    (void)err;
    (void)message;
    ESP_LOGE(tag, "[sock=%d]: %s\n"
                  "error=%d: %s", sock, message, err, strerror(err));
}

/**
 * @brief Tries to receive data from specified sockets in a non-blocking way,
 *        i.e. returns immediately if no data.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket for reception
 * @param[out] data Data pointer to write the received data
 * @param[in] max_len Maximum size of the allocated space for receiving data
 * @return
 *          >0 : Size of received data
 *          =0 : No data available
 *          -1 : Error occurred during socket read operation
 *          -2 : Socket is not connected, to distinguish between an actual socket error and active disconnection
 */
static int try_receive(const char *tag, const int sock, char * data, size_t max_len)
{
    int len = recv(sock, data, max_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        if (errno == ENOTCONN) {
            ESP_LOGW(tag, "[sock=%d]: Connection closed", sock);
            return -2;  // Socket has been disconnected
        }
        log_socket_error(tag, sock, errno, "Error occurred during receiving");
        return -1;
    }

    return len;
}

/**
 * @brief Sends the specified data to the socket. This function blocks until all bytes got sent.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket to write data
 * @param[in] data Data to be written
 * @param[in] len Length of the data
 * @return
 *          >0 : Size the written data
 *          -1 : Error occurred during socket write operation
 */
static int socket_send(const char *tag, const int sock, const char * data, const size_t len)
{
    int to_write = len;
    while (to_write > 0) {
        int written = send(sock, data + (len - to_write), to_write, 0);
        if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            log_socket_error(tag, sock, errno, "Error occurred during sending");
            return -1;
        }
        to_write -= written;
    }
    return len;
}

/**
 * @brief Returns the string representation of client's address (accepted on this server)
 */
static inline char* get_clients_address(struct sockaddr_storage *source_addr)
{
    static char address_str[128];
    char *res = NULL;
    // Convert ip address to string
    if (source_addr->ss_family == PF_INET) {
        res = inet_ntoa_r(((struct sockaddr_in *)source_addr)->sin_addr, address_str, sizeof(address_str) - 1);
    }
#ifdef CONFIG_LWIP_IPV6
    else if (source_addr->ss_family == PF_INET6) {
        res = inet6_ntoa_r(((struct sockaddr_in6 *)source_addr)->sin6_addr, address_str, sizeof(address_str) - 1);
    }
#endif
    if (!res) {
        address_str[0] = '\0'; // Returns empty string if conversion didn't succeed
    }
    return address_str;
}

static void tcp_server_task(void)
{
    static char rx_buffer[128];
    static const char *TAG = "nonblocking-socket-server";
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;
    int listen_sock = INVALID_SOCK;
    const int max_socks = CONFIG_LWIP_MAX_SOCKETS - 1;
    static int sock[CONFIG_LWIP_MAX_SOCKETS - 1];

    // Prepare a list of file descriptors to hold client's sockets, mark all of them as invalid, i.e. available
    for (int i=0; i<max_socks; ++i) {
        sock[i] = INVALID_SOCK;
    }

    // Translating the hostname or a string representation of an IP to address_info
    int res = getaddrinfo(HOST_IP_ADDR, PORT, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(TAG, "couldn't get hostname for `%s` "
                      "getaddrinfo() returns %d", HOST_IP_ADDR, res);
        goto error;
    }

    // Creating a listener socket
    listen_sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);

    if (listen_sock < 0) {
        log_socket_error(TAG, listen_sock, errno, "Unable to create socket");
        goto error;
    }
    ESP_LOGI(TAG, "Listener socket created");

    // Marking the socket as non-blocking
    int flags = fcntl(listen_sock, F_GETFL, 0);
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_socket_error(TAG, listen_sock, errno, "Unable to set socket non blocking");
        goto error;
    }
    ESP_LOGI(TAG, "Socket marked as non blocking");

    // Binding socket to the given address
    int err = bind(listen_sock, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) {
        log_socket_error(TAG, listen_sock, errno, "Socket unable to bind");
        goto error;
    }
    ESP_LOGI(TAG, "Socket bound on %s:%s", HOST_IP_ADDR, PORT);

    // Set queue (backlog) of pending connections to one (can be more)
    err = listen(listen_sock, 100);
    if (err != 0) {
        log_socket_error(TAG, listen_sock, errno, "Error occurred during listen");
        goto error;
    }
    ESP_LOGI(TAG, "Socket listening");

    // Main loop for accepting new connections and serving all connected clients
    while (1) {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);

        // Find a free socket
        int new_sock_index = 0;
        for (new_sock_index=0; new_sock_index<max_socks; ++new_sock_index) {
            if (sock[new_sock_index] == INVALID_SOCK) {
                break;
            }
        }

        // We accept a new connection only if we have a free socket
        if (new_sock_index < max_socks) {
            // Try to accept a new connections
            sock[new_sock_index] = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

            if (sock[new_sock_index] < 0) {
                if (errno == EWOULDBLOCK) { // The listener socket did not accepts any connection
                                            // continue to serve open connections and try to accept again upon the next iteration
                    ESP_LOGV(TAG, "No pending connections...");
                } else {
                    log_socket_error(TAG, listen_sock, errno, "Error when accepting connection");
                    goto error;
                }
            } else {
                // We have a new client connected -> print it's address
                ESP_LOGI(TAG, "[sock=%d]: Connection accepted from IP:%s", sock[new_sock_index], get_clients_address(&source_addr));

                // ...and set the client's socket non-blocking
                flags = fcntl(sock[new_sock_index], F_GETFL, 0);
                if (fcntl(sock[new_sock_index], F_SETFL, flags | O_NONBLOCK) == -1) {
                    log_socket_error(TAG, sock[new_sock_index], errno, "Unable to set socket non blocking");
                    goto error;
                }
                ESP_LOGI(TAG, "[sock=%d]: Socket marked as non blocking", sock[new_sock_index]);
            }
        }

        // We serve all the connected clients in this loop
        for (int i=0; i<max_socks; ++i) {
            if (sock[i] != INVALID_SOCK) {

                // This is an open socket -> try to serve it
                int len = try_receive(TAG, sock[i], rx_buffer, sizeof(rx_buffer));
                if (len < 0) {
                    // Error occurred within this client's socket -> close and mark invalid
                    ESP_LOGI(TAG, "[sock=%d]: try_receive() returned %d -> closing the socket", sock[i], len);
                    close(sock[i]);
                    sock[i] = INVALID_SOCK;
                } else if (len > 0) {
                    // Received some data -> echo back
                    ESP_LOGI(TAG, "[sock=%d]: Received %.*s", sock[i], len, rx_buffer);

                    len = socket_send(TAG, sock[i], rx_buffer, len);
                    if (len < 0) {
                        // Error occurred on write to this socket -> close it and mark invalid
                        ESP_LOGI(TAG, "[sock=%d]: socket_send() returned %d -> closing the socket", sock[i], len);
                        close(sock[i]);
                        sock[i] = INVALID_SOCK;
                    } else {
                        // Successfully echoed to this socket
                        ESP_LOGI(TAG, "[sock=%d]: Written %.*s", sock[i], len, rx_buffer);
                    }
                }

            } // one client's socket
        } // for all sockets

        // Yield to other tasks
        usleep(0);
    }

error:
    if (listen_sock != INVALID_SOCK) {
        close(listen_sock);
    }

    for (int i=0; i<max_socks; ++i) {
        if (sock[i] != INVALID_SOCK) {
            close(sock[i]);
        }
    }

    free(address_info);
    // vTaskDelete(NULL);
}


#ifdef ESP_LWIP

#if LWIP_NETIF_STATUS_CALLBACK
static void
status_callback(struct netif *state_netif)
{
  if (netif_is_up(state_netif)) {
#if LWIP_IPV4
    printf("status_callback==UP, local interface IP is %s\n", ip4addr_ntoa(netif_ip4_addr(state_netif)));
#else
    printf("status_callback==UP\n");
#endif
  } else {
    printf("status_callback==DOWN\n");
  }
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
static void
link_callback(struct netif *state_netif)
{
  if (netif_is_link_up(state_netif)) {
    printf("link_callback==UP\n");
  } else {
    printf("link_callback==DOWN\n");
  }
}
#endif /* LWIP_NETIF_LINK_CALLBACK */



static void
test_netif_init(void)
{


#if LWIP_IPV4
  ip4_addr_t ipaddr, netmask, gw;
  ip4_addr_set_zero(&gw);
  ip4_addr_set_zero(&ipaddr);
  ip4_addr_set_zero(&netmask);

  LWIP_PORT_INIT_GW(&gw);
  LWIP_PORT_INIT_IPADDR(&ipaddr);
  LWIP_PORT_INIT_NETMASK(&netmask);
  printf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));

#if LWIP_IPV4
  init_default_netif(&ipaddr, &netmask, &gw);
#else
  init_default_netif();
#endif
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif_default, 1);
#if LWIP_IPV6_AUTOCONFIG
  netif_default->ip6_autoconfig_enabled = 1;
#endif
  printf("ip6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(netif_default, 0)));
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(netif_default, status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(netif_default, link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */

  netif_set_up(netif_default);

#endif /* USE_ETHERNET */
}

static void
test_init(void * arg)
{ /* remove compiler warning */
  sys_sem_t *init_sem;
  LWIP_ASSERT("arg != NULL", arg != NULL);
  init_sem = (sys_sem_t*)arg;

  /* init randomizer again (seed per thread) */
  srand((unsigned int)time(0));

  /* init network interfaces */
  test_netif_init();

  sys_sem_signal(init_sem);
}
#endif

int main (void)
{
#ifdef ESP_LWIP
  err_t err;
  sys_sem_t init_sem;

  setvbuf(stdout, NULL,_IONBF, 0);
  err = sys_sem_new(&init_sem, 0);
  LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
  LWIP_UNUSED_ARG(err);
  tcpip_init(test_init, &init_sem);
  sys_sem_wait(&init_sem);
  sys_sem_free(&init_sem);
#endif
    tcp_server_task();
    // tcp_client_task();
    // tcp_async_connect();
    // tcp_bind_test();
    return 0;
}