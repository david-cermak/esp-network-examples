#ifdef ESP_LWIP
#include "lwip/opt.h"
#endif

#if LWIP_SOCKET
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwipcfg.h"
#include "default_netif.h"
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


#define ESP_LOGI(tag, fmt, ...) do { printf("(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
#define ESP_LOGE(tag, fmt, ...) do { printf("ERROR(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
// #define ESP_LOGI(tag, fmt, ...)
// #define ESP_LOGE(tag, fmt, ...)
#define HOST_IP_ADDR "192.168.1.1"
// #define HOST_IP_ADDR "0.0.0.0"
#define PORT 3333

static const char *TAG = "tcp-client";
static const char *payload = "Message from ESP32 ";

void* non_block_sock_thread(void *arg);
void* block_sock_thread(void *arg);
/********************  non-blocing connect/read  *********************************************/
void* non_block_sock_thread(void *arg)
{
    char rx_buffer[128];

    int fd = *((int*)arg);
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	// servaddr.sin_addr.s_addr = htonl(0x12345678);
    servaddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);

	servaddr.sin_port = htons(PORT);

    // set to non-blocking mode
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        ESP_LOGE(TAG, "[sock=%d] get file flags error: %s", fd, strerror(errno));
        return NULL;
    }
    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) < 0) {
        ESP_LOGE(TAG, "[sock=%d] set blocking/nonblocking error: %s", fd, strerror(errno));
        return NULL;
    }
    ESP_LOGI(TAG, "[sock=%d] Connecting to server. HOST: %s, Port: %d", fd, HOST_IP_ADDR, PORT);
    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
        if (errno == EINPROGRESS) {
            fd_set fdset;
            struct timeval tv = { .tv_usec = 0, .tv_sec = 20 }; // Default connection timeout is 10 s

            FD_ZERO(&fdset);
            FD_SET(fd, &fdset);

            int res = select(fd+1, NULL, &fdset, NULL, &tv);
            if (res < 0) {
                ESP_LOGE(TAG, "[sock=%d] select() error: %s", fd, strerror(errno));
                return NULL;
            }
            else if (res == 0) {
                ESP_LOGE(TAG, "[sock=%d] select() timeout", fd);
                return NULL;
            } else {
                int sockerr;
                socklen_t len = (socklen_t)sizeof(int);

                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                    ESP_LOGE(TAG, "[sock=%d] getsockopt() error: %s", fd, strerror(errno));
                    return NULL;
                }
                else if (sockerr) {
                    ESP_LOGE(TAG, "[sock=%d] delayed connect error: %s", fd, strerror(sockerr));
                    return NULL;

                }
            }
        } else {
            ESP_LOGE(TAG, "[sock=%d] connect() error: %s", fd, strerror(errno));
            return NULL;
        }
    }
    ESP_LOGI(TAG, "[sock=%d] Connected", fd);
    int ret = recv(fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (ret < 0 && (errno == EINPROGRESS || errno == EAGAIN)) {
        fd_set fdset;
        fd_set errset;
        struct timeval tv = { .tv_usec = 0, .tv_sec = 20 };

        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        FD_ZERO(&errset);
        FD_SET(fd, &errset);

        int res = select(fd+1, &fdset,  NULL, &errset, &tv);
        if (res < 0) {
            ESP_LOGE(TAG, "[sock=%d] select() error: %s", fd, strerror(errno));
            return NULL;
        } else if (res == 0) {
            ESP_LOGE(TAG, "[sock=%d] select() timeout", fd);
            return NULL;
        } else {
            int sockerr;
            socklen_t len = (socklen_t)sizeof(int);
            if (FD_ISSET(fd, &errset)) {
                ESP_LOGE(TAG, "[sock=%d] Select errset", fd);
            }
            if (FD_ISSET(fd, &fdset)) {
                ESP_LOGI(TAG, "[sock=%d] Select readset", fd);
                ret = recv(fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
                if (ret < 0) {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                } else {
                    rx_buffer[ret] = 0;
                    ESP_LOGI(TAG, "Received %d bytes from %s:", ret, HOST_IP_ADDR);
                    ESP_LOGI(TAG, "%s", rx_buffer);
                }
            }
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                ESP_LOGE(TAG, "[sock=%d] getsockopt() error: %s", fd, strerror(errno));
                return NULL;
            } else if (sockerr) {
                ESP_LOGE(TAG, "[sock=%d] delayed connect error: %s", fd, strerror(sockerr));
                return NULL;
            }

        }
    } else {
        ESP_LOGE(TAG, "[sock=%d] read() returns: %d error(%d): %s", fd, ret, errno, strerror(errno));
        return NULL;
    }

    return NULL;

}

/********************  blocing connect/read  *********************************************/

void* block_sock_thread(void *arg)
{
    char rx_buffer[128];

    int sockfd = *((int*)arg);
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	// servaddr.sin_addr.s_addr = htonl(0x12345678);
    servaddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);

	servaddr.sin_port = htons(PORT);
	// connect the client socket to server socket
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
        return NULL;
	} else{
		printf("connected to the server..\n");
	}

    struct timeval tv = { .tv_usec = 0, .tv_sec = 0 };
    struct timeval *ptv = &tv;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, ptv, sizeof(tv)) != 0) {
        ESP_LOGE(TAG, "Fail to setsockopt SO_RCVTIMEO");
        return NULL;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, ptv, sizeof(tv)) != 0) {
        ESP_LOGE(TAG, "Fail to setsockopt SO_SNDTIMEO");
        return NULL;
    }
    int len = recv(sockfd, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else {
        rx_buffer[len] = 0;
        ESP_LOGI(TAG, "Received %d bytes from %s:", len, HOST_IP_ADDR);
        ESP_LOGI(TAG, "%s", rx_buffer);
    }

    return NULL;
}

void tcp_async_connect(void);
void tcp_async_connect(void)
{
    pthread_t thread;

	// socket create and varification
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}



    printf("Trying to connect...\n");
    // pthread_create(&thread, NULL, block_sock_thread, (void*)&sockfd);
    pthread_create(&thread, NULL, non_block_sock_thread, (void*)&sockfd);
    sys_msleep(2000);
    printf("closing the fd=%d...\n", sockfd);
    // shutdown(sockfd, SHUT_RDWR);
    // printf("Shutsown fd=%d...\n", sockfd);
    // sys_msleep(10000);
    close(sockfd);
    printf("CLOSED fd=%d...\n", sockfd);
    pthread_join(thread, NULL);

}

/*****************************************************************/
void tcp_bind_test(void);
void tcp_bind_test(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);

    for (int i = 0; i< 1000; ++i) {
        dest_addr.sin_port = htons(PORT + i);
        int sock =  socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "%d: Successfully bound on port %d", i, PORT + i);
        usleep(10000);
    }

}

void tcp_client_task(void);
void tcp_client_task(void)
{
    char rx_buffer[128];
    (void)TAG;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_port = htons(PORT);
        int sock =  socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            err = send(sock, payload, strlen(payload), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, HOST_IP_ADDR);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

            usleep(200000);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

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
    // tcp_client_task();
    tcp_async_connect();
    // tcp_bind_test();
    return 0;
}