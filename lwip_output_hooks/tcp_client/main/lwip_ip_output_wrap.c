/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * Demonstrates intercepting outgoing IP packets without modifying ESP-IDF lwIP.
 *
 * Linker wrap replaces ip4_output_if() / ip6_output_if() with __wrap_* variants.
 * This mirrors the proposed LWIP_HOOK_IP4/IP6_OUTPUT hooks (issue IDFGH-17754).
 *
 * Drop semantics: return ERR_RTE and leave the pbuf alone. Many lwIP callers
 * (nd6_send_ns, icmp6, icmp4, tcp_output_control_segment_netif) always call
 * pbuf_free() after ip*_output_if() regardless of the return code. Freeing the
 * pbuf inside the hook causes a double-free heap corruption on those paths.
 *
 * ip6_output() is not wrapped here: the stack calls ip6_output_if() directly
 * (via the ip_output_if macro in tcp_out.c, nd6.c, icmp6.c, etc.).
 */
#include "sdkconfig.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#if defined(CONFIG_EXAMPLE_IPV4)
#include "lwip/ip4.h"
#endif
#if defined(CONFIG_EXAMPLE_IPV6)
#include "lwip/ip6.h"
#endif

static const char *TAG = "ip_output_hook";

#if defined(CONFIG_EXAMPLE_IPV4)

extern err_t __real_ip4_output_if(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest,
                                  u8_t ttl, u8_t tos, u8_t proto, struct netif *netif);

static err_t ip4_output_filter(const ip4_addr_t *src, const ip4_addr_t *dest, u8_t proto)
{
    char src_str[IP4ADDR_STRLEN_MAX];
    char dest_str[IP4ADDR_STRLEN_MAX];

    ip4addr_ntoa_r(src, src_str, sizeof(src_str));
    ip4addr_ntoa_r(dest, dest_str, sizeof(dest_str));
    ESP_LOGI(TAG, "IPv4 output: proto=%u src=%s dest=%s", proto, src_str, dest_str);

#if CONFIG_EXAMPLE_IP4_OUTPUT_DROP_TCP
    if (proto == IP_PROTO_TCP) {
        ESP_LOGW(TAG, "IPv4 output: DROP TCP (demo filter)");
        return ERR_RTE;
    }
#endif

    return ERR_OK;
}

err_t __wrap_ip4_output_if(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest,
                           u8_t ttl, u8_t tos, u8_t proto, struct netif *netif)
{
    err_t err = ip4_output_filter(src, dest, proto);
    if (err != ERR_OK) {
        return err;
    }

    return __real_ip4_output_if(p, src, dest, ttl, tos, proto, netif);
}

#endif /* CONFIG_EXAMPLE_IPV4 */

#if defined(CONFIG_EXAMPLE_IPV6)

extern err_t __real_ip6_output_if(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
                                  u8_t hl, u8_t tc, u8_t nexth, struct netif *netif);

static err_t ip6_output_filter(const ip6_addr_t *src, const ip6_addr_t *dest, u8_t nexth)
{
    char src_str[IP6ADDR_STRLEN_MAX];
    char dest_str[IP6ADDR_STRLEN_MAX];

    ip6addr_ntoa_r(src, src_str, sizeof(src_str));
    ip6addr_ntoa_r(dest, dest_str, sizeof(dest_str));
    ESP_LOGI(TAG, "IPv6 output: nexth=%u src=%s dest=%s", nexth, src_str, dest_str);

#if CONFIG_EXAMPLE_IP6_OUTPUT_DROP_TCP
    if (nexth == IP6_NEXTH_TCP) {
        ESP_LOGW(TAG, "IPv6 output: DROP TCP (demo filter)");
        return ERR_RTE;
    }
#endif

    return ERR_OK;
}

err_t __wrap_ip6_output_if(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
                           u8_t hl, u8_t tc, u8_t nexth, struct netif *netif)
{
    err_t err = ip6_output_filter(src, dest, nexth);
    if (err != ERR_OK) {
        return err;
    }

    return __real_ip6_output_if(p, src, dest, hl, tc, nexth, netif);
}

#endif /* CONFIG_EXAMPLE_IPV6 */
