// Minimal AEAD UDP receiver demo (algorithm selectable via Kconfig)
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "crypto_aead/crypto_aead.h"
#include "esp_timer.h"

#define TAG "AEAD_RECV"

#ifndef CONFIG_ASCON_LISTEN_PORT
#define LISTEN_PORT 3333
#else
#define LISTEN_PORT CONFIG_ASCON_LISTEN_PORT
#endif

static const uint8_t g_key[AEAD_KEY_LEN] = {
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B, 0x0C,0x0D,0x0E,0x0F
#if AEAD_KEY_LEN > 16
    ,0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B, 0x1C,0x1D,0x1E,0x1F
#endif
};

static void bytes_to_hex(const uint8_t* in, size_t len, char* out, size_t outlen)
{
    static const char* hex = "0123456789abcdef";
    size_t need = len * 2 + 1;
    if (outlen < need) return;
    for (size_t i = 0; i < len; ++i) {
        out[2*i] = hex[(in[i] >> 4) & 0xF];
        out[2*i+1] = hex[in[i] & 0xF];
    }
    out[2*len] = '\0';
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
    ESP_ERROR_CHECK(example_connect());

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed");
        return;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind failed on port %d", LISTEN_PORT);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "listening on UDP port %d", LISTEN_PORT);

    while (1) {
        static uint8_t buf[2048];
        struct sockaddr_in src;
        socklen_t slen = sizeof(src);
        ssize_t r = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&src, &slen);
        if (r < 0) {
            ESP_LOGE(TAG, "recvfrom failed");
            break;
        }
        if ((size_t)r < AEAD_NONCE_LEN + AEAD_TAG_LEN) {
            ESP_LOGE(TAG, "packet too short (%d)", (int)r);
            continue;
        }

        const uint8_t* nonce = buf;
        const uint8_t* ct_and_tag = buf + AEAD_NONCE_LEN;
        size_t clen = (size_t)r - AEAD_NONCE_LEN;

        // For demo, use AAD as the incrementing counter; we cannot know it,
        // but sender used counter embedded in nonce high 8 bytes. Reuse those.
        static uint8_t aad[8];
        memcpy(aad, nonce, 8);

        static uint8_t pt[1024];
        size_t ptlen = 0;
        int64_t t0 = esp_timer_get_time();
        int dec = aead_decrypt(pt, &ptlen, ct_and_tag, clen,
                               aad, sizeof(aad), nonce, g_key);
        int64_t t1 = esp_timer_get_time();
        static char nonce_hex[AEAD_NONCE_LEN*2+1];
        bytes_to_hex(nonce, AEAD_NONCE_LEN, nonce_hex, sizeof(nonce_hex));

        static char ct_hex[1024];
        bytes_to_hex(ct_and_tag, clen, ct_hex, sizeof(ct_hex));

        if (dec == 0) {
            // Ensure null-terminated for printing
            size_t print_len = ptlen < sizeof(pt)-1 ? ptlen : sizeof(pt)-1;
            pt[print_len] = '\0';
            ESP_LOGI(TAG, "from %s:%d nonce=%s ct|tag=%s pt=\"%s\" dec_us=%" PRId64,
                     inet_ntoa(src.sin_addr), ntohs(src.sin_port),
                     nonce_hex, ct_hex, (char*)pt, (t1 - t0));
        } else {
            ESP_LOGE(TAG, "auth failure from %s:%d nonce=%s ct|tag=%s",
                     inet_ntoa(src.sin_addr), ntohs(src.sin_port),
                     nonce_hex, ct_hex);
        }
    }

    close(sock);
}
