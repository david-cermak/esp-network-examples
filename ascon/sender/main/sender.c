// Minimal AEAD UDP sender demo (algorithm selectable via Kconfig)
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "AEAD_SENDER"

// Configure receiver IP and port here
#ifndef CONFIG_ASCON_RECEIVER_IP
#define RECEIVER_IP "192.168.0.35"
#else
#define RECEIVER_IP CONFIG_ASCON_RECEIVER_IP
#endif

#ifndef CONFIG_ASCON_RECEIVER_PORT
#define RECEIVER_PORT 3333
#else
#define RECEIVER_PORT CONFIG_ASCON_RECEIVER_PORT
#endif

static const uint8_t g_key[AEAD_KEY_LEN] = {
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B, 0x0C,0x0D,0x0E,0x0F
#if AEAD_KEY_LEN > 16
    ,0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B, 0x1C,0x1D,0x1E,0x1F
#endif
};

static void be64(uint8_t out[8], uint64_t x)
{
    for (int i = 7; i >= 0; --i) { out[i] = (uint8_t)(x & 0xFF); x >>= 8; }
}

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
    // return;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed");
        return;
    }

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(RECEIVER_PORT);
    if (inet_aton(RECEIVER_IP, &dest.sin_addr) == 0) {
        ESP_LOGE(TAG, "Invalid receiver IP: %s", RECEIVER_IP);
        close(sock);
        return;
    }

    uint64_t counter = 1;
    const char* plaintext = "Hello from ESP32!";

    while (1) {
        static uint8_t nonce[AEAD_NONCE_LEN] = {0};
        be64(nonce, counter);

        uint8_t aad[8];
        be64(aad, counter);

        const uint8_t* m = (const uint8_t*)plaintext;
        size_t mlen = strlen(plaintext);

        static uint8_t ct_and_tag[512];
        size_t clen = 0;
        int64_t t0 = esp_timer_get_time();
        int enc = aead_encrypt(ct_and_tag, &clen, m, mlen,
                               aad, sizeof(aad),
                               nonce, g_key);
        int64_t t1 = esp_timer_get_time();
        if (enc != 0) {
            ESP_LOGE(TAG, "ascon encrypt failed");
            break;
        }

        static uint8_t packet[AEAD_NONCE_LEN + 512];
        if (clen > sizeof(ct_and_tag)) {
            ESP_LOGE(TAG, "ciphertext too large");
            break;
        }
        memcpy(packet, nonce, AEAD_NONCE_LEN);
        memcpy(packet + AEAD_NONCE_LEN, ct_and_tag, clen);

        ssize_t sent = sendto(sock, packet, AEAD_NONCE_LEN + clen, 0,
                              (struct sockaddr*)&dest, sizeof(dest));
        if (sent < 0) {
            ESP_LOGE(TAG, "sendto failed");
            break;
        }

        static char nonce_hex[AEAD_NONCE_LEN*2+1];
        static  char ct_hex[1024];
        bytes_to_hex(nonce, AEAD_NONCE_LEN, nonce_hex, sizeof(nonce_hex));
        bytes_to_hex(ct_and_tag, clen, ct_hex, sizeof(ct_hex));
        ESP_LOGI(TAG, "ctr=%" PRIu64 " nonce=%s pt=\"%s\" ct|tag=%s enc_us=%" PRId64,
                 counter, nonce_hex, plaintext, ct_hex, (t1 - t0));

        counter++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    close(sock);
}
