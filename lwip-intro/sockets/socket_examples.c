
// #include "socket_examples.h"

#include "lwip/opt.h"

#if LWIP_SOCKET && (LWIP_IPV4 || LWIP_IPV6)

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"

#include "lwipcfg.h"
#include "default_netif.h"

#include <time.h>


#include <string.h>
#include <stdio.h>

#ifndef SOCK_TARGET_HOST4
#define SOCK_TARGET_HOST4  "192.168.0.1"
#endif

#ifndef SOCK_TARGET_HOST6
#define SOCK_TARGET_HOST6  "FE80::12:34FF:FE56:78AB"
#endif

#ifndef SOCK_TARGET_PORT
#define SOCK_TARGET_PORT  80
#endif

#ifndef SOCK_TARGET_MAXHTTPPAGESIZE
#define SOCK_TARGET_MAXHTTPPAGESIZE 1024
#endif

#ifndef SOCKET_EXAMPLES_RUN_PARALLEL
#define SOCKET_EXAMPLES_RUN_PARALLEL 0
#endif

const u8_t cmpbuf[8] = {0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab};

/* a helper struct to ensure memory before/after fd_set is not touched */
typedef struct _xx
{
  u8_t buf1[8];
  fd_set readset;
  u8_t buf2[8];
  fd_set writeset;
  u8_t buf3[8];
  fd_set errset;
  u8_t buf4[8];
} fdsets;

#define INIT_FDSETS(sets) do { \
  memset((sets)->buf1, 0xab, 8); \
  memset((sets)->buf2, 0xab, 8); \
  memset((sets)->buf3, 0xab, 8); \
  memset((sets)->buf4, 0xab, 8); \
}while(0)

#define CHECK_FDSETS(sets) do { \
  LWIP_ASSERT("buf1 fail", !memcmp((sets)->buf1, cmpbuf, 8)); \
  LWIP_ASSERT("buf2 fail", !memcmp((sets)->buf2, cmpbuf, 8)); \
  LWIP_ASSERT("buf3 fail", !memcmp((sets)->buf3, cmpbuf, 8)); \
  LWIP_ASSERT("buf4 fail", !memcmp((sets)->buf4, cmpbuf, 8)); \
}while(0)

// static ip_addr_t dstaddr;
// #define NO_ESP_LOGS
#ifdef NO_ESP_LOGS
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGE(tag, fmt, ...)
#else
static const char *TAG = "tcp-client";
#define ESP_LOGI(tag, fmt, ...) do { printf("(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
#define ESP_LOGE(tag, fmt, ...) do { printf("ERROR(%s): ", tag); printf(fmt, ##__VA_ARGS__); printf("\n"); } while(0)
#endif
#define HOST_IP_ADDR "192.168.1.1"
// #define HOST_IP_ADDR "0.0.0.0"
#define PORT 3333

static const char *payload = "Message from ESP32 ";

u32_t sio_tryread(sio_fd_t fd, u8_t* data, u32_t len);
u32_t sio_tryread(sio_fd_t fd, u8_t* data, u32_t len)
{
  LWIP_UNUSED_ARG(fd);
  LWIP_UNUSED_ARG(data);
  LWIP_UNUSED_ARG(len);
  return 0;  
}

void tcp_bind_test(void);
void tcp_bind_test(void)
{
    int sock_array[1000];
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);

    for (int i = 0; i< 100; ++i) {
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
        sock_array[i] = sock;
        // close(sock);
    }
    for (int i = 0; i< 100; ++i) {
        usleep(10000);
        close(sock_array[i]);
    }

}

void sock_echo(void *arg);

void
sock_echo(void *arg)
{
  int sock = *(int*)arg;
  char rx_buffer[128];
ESP_LOGE(TAG, "SOCKET= %d", sock);
  while (1) {
      int err = send(sock, payload, strlen(payload), 0);
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

}

void sock_data(void *arg);
void
sock_data(void *arg)
{
  int sock = *(int*)arg;


    while (1) {
      int err = send(sock, payload, strlen(payload), 0);
      if (err < 0) {
          ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
          break;
      }
      usleep(1000000);
  }
}


int sock_connect(void);
int sock_connect(void)
{
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
  dest_addr.sin_port = htons(PORT);
  ESP_LOGI(TAG, "CREATE SOCKET!!!");
  int sock =  socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      return -1;
  }
  ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

  int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
  if (err != 0) {
      ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
      return -1;
  }
  ESP_LOGI(TAG, "Successfully connected");
  return sock;
}


void tcp_client_task(void *pvParameters);
void tcp_client_task(void *pvParameters)
{
  LWIP_UNUSED_ARG(pvParameters);
    char rx_buffer[128];
    int run = 1;

    while(run)
    {
      run = 0;

        struct sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_port = htons(PORT);
        ESP_LOGI(TAG, "CREATE SOCKET!!!");
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

/** This is an example function that tests
    blocking- and nonblocking connect. */
void
sockex_nonblocking_connect(void *arg);

void
sockex_nonblocking_connect(void *arg)
{
#if LWIP_SOCKET_SELECT
  int s;
  int ret;
  int opt;
#if LWIP_IPV6
  struct sockaddr_in6 addr;
#else /* LWIP_IPV6 */
  struct sockaddr_in addr;
#endif /* LWIP_IPV6 */
  fdsets sets;
  struct timeval tv;
  u32_t ticks_a, ticks_b;
  int err;
  const ip_addr_t *ipaddr = (const ip_addr_t*)arg;
  struct pollfd fds;
  INIT_FDSETS(&sets);

  /* set up address to connect to */
  memset(&addr, 0, sizeof(addr));
#if LWIP_IPV6
  addr.sin6_len = sizeof(addr);
  addr.sin6_family = AF_INET6;
  addr.sin6_port = PP_HTONS(SOCK_TARGET_PORT);
  inet6_addr_from_ip6addr(&addr.sin6_addr, ip_2_ip6(ipaddr));
#else /* LWIP_IPV6 */
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = PP_HTONS(SOCK_TARGET_PORT);
  inet_addr_from_ip4addr(&addr.sin_addr, ip_2_ip4(ipaddr));
#endif /* LWIP_IPV6 */

  /* first try blocking: */

  /* create the socket */
#if LWIP_IPV6
  s = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s >= 0", s >= 0);

  /* connect */
  ret = lwip_connect(s, (struct sockaddr*)&addr, sizeof(addr));
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

  /* write something */
  ret = lwip_write(s, "test", 4);
  LWIP_ASSERT("ret == 4", ret == 4);

  /* close */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);

  /* now try nonblocking and close before being connected */

  /* create the socket */
#if LWIP_IPV6
  s = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s >= 0", s >= 0);
  /* nonblocking */
  opt = lwip_fcntl(s, F_GETFL, 0);
  LWIP_ASSERT("ret != -1", ret != -1);
  opt |= O_NONBLOCK;
  ret = lwip_fcntl(s, F_SETFL, opt);
  LWIP_ASSERT("ret != -1", ret != -1);
  /* connect */
  ret = lwip_connect(s, (struct sockaddr*)&addr, sizeof(addr));
  /* should have an error: "inprogress" */
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EINPROGRESS", err == EINPROGRESS);
  /* close */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);
  /* try to close again, should fail with EBADF */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EBADF", err == EBADF);
  printf("closing socket in nonblocking connect succeeded\n");

  /* now try nonblocking, connect should succeed:
     this test only works if it is fast enough, i.e. no breakpoints, please! */

  /* create the socket */
#if LWIP_IPV6
  s = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s >= 0", s >= 0);

  /* nonblocking */
  opt = 1;
  ret = lwip_ioctl(s, FIONBIO, &opt);
  LWIP_ASSERT("ret == 0", ret == 0);

  /* connect */
  ret = lwip_connect(s, (struct sockaddr*)&addr, sizeof(addr));
  /* should have an error: "inprogress" */
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EINPROGRESS", err == EINPROGRESS);

  /* write should fail, too */
  ret = lwip_write(s, "test", 4);
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EINPROGRESS", err == EINPROGRESS);

  CHECK_FDSETS(&sets);
  FD_ZERO(&sets.readset);
  CHECK_FDSETS(&sets);
  FD_SET(s, &sets.readset);
  CHECK_FDSETS(&sets);
  FD_ZERO(&sets.writeset);
  CHECK_FDSETS(&sets);
  FD_SET(s, &sets.writeset);
  CHECK_FDSETS(&sets);
  FD_ZERO(&sets.errset);
  CHECK_FDSETS(&sets);
  FD_SET(s, &sets.errset);
  CHECK_FDSETS(&sets);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  /* select without waiting should fail */
  ret = lwip_select(s + 1, &sets.readset, &sets.writeset, &sets.errset, &tv);
  CHECK_FDSETS(&sets);
  LWIP_ASSERT("ret == 0", ret == 0);
  LWIP_ASSERT("!FD_ISSET(s, &writeset)", !FD_ISSET(s, &sets.writeset));
  LWIP_ASSERT("!FD_ISSET(s, &readset)", !FD_ISSET(s, &sets.readset));
  LWIP_ASSERT("!FD_ISSET(s, &errset)", !FD_ISSET(s, &sets.errset));

  fds.fd = s;
  fds.events = POLLIN|POLLOUT;
  fds.revents = 0;
  ret = lwip_poll(&fds, 1, 0);
  LWIP_ASSERT("ret == 0", ret == 0);
  LWIP_ASSERT("fds.revents == 0", fds.revents == 0);

  FD_ZERO(&sets.readset);
  FD_SET(s, &sets.readset);
  FD_ZERO(&sets.writeset);
  FD_SET(s, &sets.writeset);
  FD_ZERO(&sets.errset);
  FD_SET(s, &sets.errset);
  ticks_a = sys_now();
  /* select with waiting should succeed */
  ret = lwip_select(s + 1, &sets.readset, &sets.writeset, &sets.errset, NULL);
  ticks_b = sys_now();
  LWIP_ASSERT("ret == 1", ret == 1);
  LWIP_ASSERT("FD_ISSET(s, &writeset)", FD_ISSET(s, &sets.writeset));
  LWIP_ASSERT("!FD_ISSET(s, &readset)", !FD_ISSET(s, &sets.readset));
  LWIP_ASSERT("!FD_ISSET(s, &errset)", !FD_ISSET(s, &sets.errset));

  fds.fd = s;
  fds.events = POLLIN|POLLOUT;
  fds.revents = 0;
  ret = lwip_poll(&fds, 1, 0);
  LWIP_ASSERT("ret == 1", ret == 1);
  LWIP_ASSERT("fds.revents & POLLOUT", fds.revents & POLLOUT);

  /* now write should succeed */
  ret = lwip_write(s, "test", 4);
  LWIP_ASSERT("ret == 4", ret == 4);

  /* close */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);

  printf("select() needed %d ticks to return writable\n", (int)(ticks_b - ticks_a));


  /* now try nonblocking to invalid address:
     this test only works if it is fast enough, i.e. no breakpoints, please! */

  /* create the socket */
#if LWIP_IPV6
  s = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s >= 0", s >= 0);

  /* nonblocking */
  opt = 1;
  ret = lwip_ioctl(s, FIONBIO, &opt);
  LWIP_ASSERT("ret == 0", ret == 0);

#if LWIP_IPV6
  addr.sin6_addr.un.u8_addr[0]++; /* this should result in an invalid address */
#else /* LWIP_IPV6 */
  addr.sin_addr.s_addr++; /* this should result in an invalid address */
#endif /* LWIP_IPV6 */

  /* connect */
  ret = lwip_connect(s, (struct sockaddr*)&addr, sizeof(addr));
  /* should have an error: "inprogress" */
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EINPROGRESS", err == EINPROGRESS);

  /* write should fail, too */
  ret = lwip_write(s, "test", 4);
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EINPROGRESS", err == EINPROGRESS);
  LWIP_UNUSED_ARG(err);

  FD_ZERO(&sets.readset);
  FD_SET(s, &sets.readset);
  FD_ZERO(&sets.writeset);
  FD_SET(s, &sets.writeset);
  FD_ZERO(&sets.errset);
  FD_SET(s, &sets.errset);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  /* select without waiting should fail */
  ret = lwip_select(s + 1, &sets.readset, &sets.writeset, &sets.errset, &tv);
  LWIP_ASSERT("ret == 0", ret == 0);

  FD_ZERO(&sets.readset);
  FD_SET(s, &sets.readset);
  FD_ZERO(&sets.writeset);
  FD_SET(s, &sets.writeset);
  FD_ZERO(&sets.errset);
  FD_SET(s, &sets.errset);
  ticks_a = sys_now();
  /* select with waiting should eventually succeed and return errset! */
  ret = lwip_select(s + 1, &sets.readset, &sets.writeset, &sets.errset, NULL);
  ticks_b = sys_now();
  LWIP_ASSERT("ret > 0", ret > 0);
  LWIP_ASSERT("FD_ISSET(s, &errset)", FD_ISSET(s, &sets.errset));
  /*LWIP_ASSERT("!FD_ISSET(s, &readset)", !FD_ISSET(s, &sets.readset));
  LWIP_ASSERT("!FD_ISSET(s, &writeset)", !FD_ISSET(s, &sets.writeset));*/

  /* close */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);
  LWIP_UNUSED_ARG(ret);

  printf("select() needed %d ticks to return error\n", (int)(ticks_b - ticks_a));
  printf("sockex_nonblocking_connect finished successfully\n");
#else
  LWIP_UNUSED_ARG(arg);
#endif
}

/** This is an example function that tests
    the recv function (timeout etc.). */
void
sockex_testrecv(void *arg);

void
sockex_testrecv(void *arg)
{
  int s;
  int ret;
  int err;
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int opt, opt2;
#else
  struct timeval opt, opt2;
#endif
  socklen_t opt2size;
#if LWIP_IPV6
  struct sockaddr_in6 addr;
#else /* LWIP_IPV6 */
  struct sockaddr_in addr;
#endif /* LWIP_IPV6 */
  size_t len;
  char rxbuf[SOCK_TARGET_MAXHTTPPAGESIZE];
#if LWIP_SOCKET_SELECT
  fd_set readset;
  fd_set errset;
  struct timeval tv;
#endif
  const ip_addr_t *ipaddr = (const ip_addr_t*)arg;

  /* set up address to connect to */
  memset(&addr, 0, sizeof(addr));
#if LWIP_IPV6
  addr.sin6_len = sizeof(addr);
  addr.sin6_family = AF_INET6;
  addr.sin6_port = PP_HTONS(SOCK_TARGET_PORT);
  inet6_addr_from_ip6addr(&addr.sin6_addr, ip_2_ip6(ipaddr));
#else /* LWIP_IPV6 */
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = PP_HTONS(SOCK_TARGET_PORT);
  inet_addr_from_ip4addr(&addr.sin_addr, ip_2_ip4(ipaddr));
#endif /* LWIP_IPV6 */

  /* first try blocking: */

  /* create the socket */
#if LWIP_IPV6
  s = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s >= 0", s >= 0);

  /* connect */
  ret = lwip_connect(s, (struct sockaddr*)&addr, sizeof(addr));
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

  /* set recv timeout (100 ms) */
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  opt = 100;
#else
  opt.tv_sec = 0;
  opt.tv_usec = 100 * 1000;
#endif
  ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt));
  LWIP_ASSERT("ret == 0", ret == 0);
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  opt2 = 0;
#else
  opt2.tv_sec = 0;
  opt2.tv_usec = 0;
#endif
  opt2size = sizeof(opt2);
  ret = lwip_getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt2, &opt2size);
  LWIP_ASSERT("ret == 0", ret == 0);
  LWIP_ASSERT("opt2size == sizeof(opt2)", opt2size == sizeof(opt2));
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  LWIP_ASSERT("opt == opt2", opt == opt2);
#else
  LWIP_ASSERT("opt == opt2", opt.tv_sec == opt2.tv_sec);
  LWIP_ASSERT("opt == opt2", opt.tv_usec == opt2.tv_usec);
#endif

  /* write the start of a GET request */
#define SNDSTR1 "G"
  len = strlen(SNDSTR1);
  ret = lwip_write(s, SNDSTR1, len);
  LWIP_ASSERT("ret == len", ret == (int)len);

  /* should time out if the other side is a good HTTP server */
  ret = lwip_read(s, rxbuf, 1);
  LWIP_ASSERT("ret == -1", ret == -1);
  err = errno;
  LWIP_ASSERT("errno == EAGAIN", err == EAGAIN);
  LWIP_UNUSED_ARG(err);

  /* write the rest of a GET request */
#define SNDSTR2 "ET / HTTP_1.1\r\n\r\n"
  len = strlen(SNDSTR2);
  ret = lwip_write(s, SNDSTR2, len);
  LWIP_ASSERT("ret == len", ret == (int)len);

  /* wait a while: should be enough for the server to send a response */
  sys_msleep(1000);

  /* should not time out but receive a response */
  ret = lwip_read(s, rxbuf, SOCK_TARGET_MAXHTTPPAGESIZE);
  LWIP_ASSERT("ret > 0", ret > 0);

#if LWIP_SOCKET_SELECT
  /* now select should directly return because the socket is readable */
  FD_ZERO(&readset);
  FD_ZERO(&errset);
  FD_SET(s, &readset);
  FD_SET(s, &errset);
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  ret = lwip_select(s + 1, &readset, NULL, &errset, &tv);
  LWIP_ASSERT("ret == 1", ret == 1);
  LWIP_ASSERT("!FD_ISSET(s, &errset)", !FD_ISSET(s, &errset));
  LWIP_ASSERT("FD_ISSET(s, &readset)", FD_ISSET(s, &readset));
#endif

  /* should not time out but receive a response */
  ret = lwip_read(s, rxbuf, SOCK_TARGET_MAXHTTPPAGESIZE);
  /* might receive a second packet for HTTP/1.1 servers */
  if (ret > 0) {
    /* should return 0: closed */
    ret = lwip_read(s, rxbuf, SOCK_TARGET_MAXHTTPPAGESIZE);
    LWIP_ASSERT("ret == 0", ret == 0);
  }

  /* close */
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);
  LWIP_UNUSED_ARG(ret);

  printf("sockex_testrecv finished successfully\n");
}

#if LWIP_SOCKET_SELECT
/** helper struct for the 2 functions below (multithreaded: thread-argument) */
struct sockex_select_helper {
  int socket;
  int wait_read;
  int expect_read;
  int wait_write;
  int expect_write;
  int wait_err;
  int expect_err;
  int wait_ms;
  sys_sem_t sem;
};

/** helper thread to wait for socket events using select */
static void
sockex_select_waiter(void *arg)
{
  struct sockex_select_helper *helper = (struct sockex_select_helper *)arg;
  int ret;
  fd_set readset;
  fd_set writeset;
  fd_set errset;
  struct timeval tv;

  LWIP_ASSERT("helper != NULL", helper != NULL);

  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_ZERO(&errset);
  if (helper->wait_read) {
    FD_SET(helper->socket, &readset);
  }
  if (helper->wait_write) {
    FD_SET(helper->socket, &writeset);
  }
  if (helper->wait_err) {
    FD_SET(helper->socket, &errset);
  }

  tv.tv_sec = helper->wait_ms / 1000;
  tv.tv_usec = (helper->wait_ms % 1000) * 1000;

  ret = lwip_select(helper->socket, &readset, &writeset, &errset, &tv);
  if (helper->expect_read || helper->expect_write || helper->expect_err) {
    LWIP_ASSERT("ret > 0", ret > 0);
  } else {
    LWIP_ASSERT("ret == 0", ret == 0);
  }
  LWIP_UNUSED_ARG(ret);
  if (helper->expect_read) {
    LWIP_ASSERT("FD_ISSET(helper->socket, &readset)", FD_ISSET(helper->socket, &readset));
  } else {
    LWIP_ASSERT("!FD_ISSET(helper->socket, &readset)", !FD_ISSET(helper->socket, &readset));
  }
  if (helper->expect_write) {
    LWIP_ASSERT("FD_ISSET(helper->socket, &writeset)", FD_ISSET(helper->socket, &writeset));
  } else {
    LWIP_ASSERT("!FD_ISSET(helper->socket, &writeset)", !FD_ISSET(helper->socket, &writeset));
  }
  if (helper->expect_err) {
    LWIP_ASSERT("FD_ISSET(helper->socket, &errset)", FD_ISSET(helper->socket, &errset));
  } else {
    LWIP_ASSERT("!FD_ISSET(helper->socket, &errset)", !FD_ISSET(helper->socket, &errset));
  }
  sys_sem_signal(&helper->sem);
}

/** This is an example function that tests
    more than one thread being active in select. */
void
sockex_testtwoselects(void *arg);

void
sockex_testtwoselects(void *arg)
{
  int s1;
  int s2;
  int ret;
#if LWIP_IPV6
  struct sockaddr_in6 addr;
#else /* LWIP_IPV6 */
  struct sockaddr_in addr;
#endif /* LWIP_IPV6 */
  size_t len;
  err_t lwiperr;
  struct sockex_select_helper h1, h2, h3, h4;
  const ip_addr_t *ipaddr = (const ip_addr_t*)arg;

  /* set up address to connect to */
  memset(&addr, 0, sizeof(addr));
#if LWIP_IPV6
  addr.sin6_len = sizeof(addr);
  addr.sin6_family = AF_INET6;
  addr.sin6_port = PP_HTONS(SOCK_TARGET_PORT);
  inet6_addr_from_ip6addr(&addr.sin6_addr, ip_2_ip6(ipaddr));
#else /* LWIP_IPV6 */
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = PP_HTONS(SOCK_TARGET_PORT);
  inet_addr_from_ip4addr(&addr.sin_addr, ip_2_ip4(ipaddr));
#endif /* LWIP_IPV6 */

  /* create the sockets */
#if LWIP_IPV6
  s1 = lwip_socket(AF_INET6, SOCK_STREAM, 0);
  s2 = lwip_socket(AF_INET6, SOCK_STREAM, 0);
#else /* LWIP_IPV6 */
  s1 = lwip_socket(AF_INET, SOCK_STREAM, 0);
  s2 = lwip_socket(AF_INET, SOCK_STREAM, 0);
#endif /* LWIP_IPV6 */
  LWIP_ASSERT("s1 >= 0", s1 >= 0);
  LWIP_ASSERT("s2 >= 0", s2 >= 0);

  /* connect, should succeed */
  ret = lwip_connect(s1, (struct sockaddr*)&addr, sizeof(addr));
  LWIP_ASSERT("ret == 0", ret == 0);
  ret = lwip_connect(s2, (struct sockaddr*)&addr, sizeof(addr));
  LWIP_ASSERT("ret == 0", ret == 0);

  /* write the start of a GET request */
#define SNDSTR1 "G"
  len = strlen(SNDSTR1);
  ret = lwip_write(s1, SNDSTR1, len);
  LWIP_ASSERT("ret == len", ret == (int)len);
  ret = lwip_write(s2, SNDSTR1, len);
  LWIP_ASSERT("ret == len", ret == (int)len);
  LWIP_UNUSED_ARG(ret);

  h1.wait_read  = 1;
  h1.wait_write = 1;
  h1.wait_err   = 1;
  h1.expect_read  = 0;
  h1.expect_write = 0;
  h1.expect_err   = 0;
  lwiperr = sys_sem_new(&h1.sem, 0);
  LWIP_ASSERT("lwiperr == ERR_OK", lwiperr == ERR_OK);
  h1.socket = s1;
  h1.wait_ms = 500;

  h2 = h1;
  lwiperr = sys_sem_new(&h2.sem, 0);
  LWIP_ASSERT("lwiperr == ERR_OK", lwiperr == ERR_OK);
  h2.socket = s2;
  h2.wait_ms = 1000;

  h3 = h1;
  lwiperr = sys_sem_new(&h3.sem, 0);
  LWIP_ASSERT("lwiperr == ERR_OK", lwiperr == ERR_OK);
  h3.socket = s2;
  h3.wait_ms = 1500;

  h4 = h1;
  lwiperr = sys_sem_new(&h4.sem, 0);
  LWIP_ASSERT("lwiperr == ERR_OK", lwiperr == ERR_OK);
  LWIP_UNUSED_ARG(lwiperr);
  h4.socket = s2;
  h4.wait_ms = 2000;

  /* select: all sockets should time out if the other side is a good HTTP server */

  sys_thread_new("sockex_select_waiter1", sockex_select_waiter, &h2, 0, 0);
  sys_msleep(100);
  sys_thread_new("sockex_select_waiter2", sockex_select_waiter, &h1, 0, 0);
  sys_msleep(100);
  sys_thread_new("sockex_select_waiter2", sockex_select_waiter, &h4, 0, 0);
  sys_msleep(100);
  sys_thread_new("sockex_select_waiter2", sockex_select_waiter, &h3, 0, 0);

  sys_sem_wait(&h1.sem);
  sys_sem_wait(&h2.sem);
  sys_sem_wait(&h3.sem);
  sys_sem_wait(&h4.sem);

  /* close */
  ret = lwip_close(s1);
  LWIP_ASSERT("ret == 0", ret == 0);
  ret = lwip_close(s2);
  LWIP_ASSERT("ret == 0", ret == 0);

  printf("sockex_testtwoselects finished successfully\n");
}
#else
static void
sockex_testtwoselects(void *arg)
{
  LWIP_UNUSED_ARG(arg);
}
#endif 

// #if !SOCKET_EXAMPLES_RUN_PARALLEL
// static void
// socket_example_test(void* arg)
// {
//   LWIP_UNUSED_ARG(arg);
//   // tcp_client_task(0);
//   sys_msleep(1000);
//   printf("all tests done, thread ending\n");
// }
// #endif

// void socket_examples_init(void);
// void socket_examples_init(void)
// {
//   int addr_ok;
// #if LWIP_IPV6
//   IP_SET_TYPE_VAL(dstaddr, IPADDR_TYPE_V6);
//   addr_ok = ip6addr_aton(SOCK_TARGET_HOST6, ip_2_ip6(&dstaddr));
// #else /* LWIP_IPV6 */
//   IP_SET_TYPE_VAL(dstaddr, IPADDR_TYPE_V4);
//   addr_ok = ip4addr_aton(SOCK_TARGET_HOST4, ip_2_ip4(&dstaddr));
// #endif /* LWIP_IPV6 */
//   LWIP_ASSERT("invalid address", addr_ok);
// #if SOCKET_EXAMPLES_RUN_PARALLEL
//   sys_thread_new("sockex_nonblocking_connect", sockex_nonblocking_connect, &dstaddr, 0, 0);
//   sys_thread_new("sockex_testrecv", sockex_testrecv, &dstaddr, 0, 0);
//   sys_thread_new("sockex_testtwoselects", sockex_testtwoselects, &dstaddr, 0, 0);
// #else
//   sys_thread_new("socket_example_test", socket_example_test, &dstaddr, 0, 0);
// #endif
// }

#ifndef USE_DEFAULT_ETH_NETIF
#define USE_DEFAULT_ETH_NETIF 1
#endif

/** Define this to 1 to enable a PPP interface. */
#ifndef USE_PPP
#define USE_PPP 0
#endif

/** Define this to 1 or 2 to support 1 or 2 SLIP interfaces. */
#ifndef USE_SLIPIF
#define USE_SLIPIF 0
#endif

/** Use an ethernet adapter? Default to enabled if port-specific ethernet netif or PPPoE are used. */
#ifndef USE_ETHERNET
#define USE_ETHERNET  (USE_DEFAULT_ETH_NETIF || PPPOE_SUPPORT)
#endif

/** Use an ethernet adapter for TCP/IP? By default only if port-specific ethernet netif is used. */
#ifndef USE_ETHERNET_TCPIP
#define USE_ETHERNET_TCPIP  (USE_DEFAULT_ETH_NETIF)
#endif


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





/* This function initializes all network interfaces */
static void
test_netif_init(void)
{


#if USE_ETHERNET
#if LWIP_IPV4
  ip4_addr_t ipaddr, netmask, gw;
  ip4_addr_set_zero(&gw);
  ip4_addr_set_zero(&ipaddr);
  ip4_addr_set_zero(&netmask);
#if USE_ETHERNET_TCPIP
  LWIP_PORT_INIT_GW(&gw);
  LWIP_PORT_INIT_IPADDR(&ipaddr);
  LWIP_PORT_INIT_NETMASK(&netmask);
  printf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));
#endif /* USE_ETHERNET_TCPIP */
#else /* LWIP_IPV4 */
  printf("Starting lwIP, IPv4 disable\n");
#endif /* LWIP_IPV4 */

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

int main(void)
{
  err_t err;

  sys_sem_t init_sem;
  setvbuf(stdout, NULL,_IONBF, 0);

  err = sys_sem_new(&init_sem, 0);
  LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
  LWIP_UNUSED_ARG(err);
  tcpip_init(test_init, &init_sem);
  /* we have to wait for initialization to finish before
   * calling update_adapter()! */
  sys_sem_wait(&init_sem);
  sys_sem_free(&init_sem);

  // int sock = sock_connect();
  // sys_thread_new("echo", sock_echo, &sock, 0, 0);
  // sys_thread_new("data", sock_data, &sock, 0, 0);

  // tcp_client_task(0);
  // sys_thread_new("sockex_select_waiter1", sockex_select_waiter, &h2, 0, 0);
  // sys_msleep(100);


  tcp_bind_test();
  // sys_msleep(10000);
  printf("all tests done, thread ending\n");
  lwip_socket_thread_cleanup();

}

#endif /* LWIP_SOCKET */
