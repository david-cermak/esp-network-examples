#include <stdio.h>
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "ethernet_init.h"
#include "esp_eth.h"
#include "netif/ppp/pppapi.h"
#include "esp_netif_net_stack.h"


static const char *TAG = "pppoe_example";

static void on_ppp_status_changed(ppp_pcb *pcb, int state, void *ctx)
{
    struct netif *netif = ctx;
    switch (state) {
        case PPPERR_NONE:
            int ip = htonl(netif->ip_addr.u_addr.ip4.addr);
            ESP_LOGI(TAG, "Connected with IP=%d.%d.%d.%d", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
            break;
        case PPPERR_CONNECT: /* Connection lost */
            ESP_LOGI(TAG, "Connection lost");
            return;
        default:
            ESP_LOGI(TAG, "Other event %d", state);
            break;
    }
}

static void setup_pppoe(esp_netif_t *netif)
{
    static struct netif s_pppif = {};
    ppp_pcb *ppp = pppapi_pppoe_create(&s_pppif, esp_netif_get_netif_impl(netif),
                                       "", "",
                                       on_ppp_status_changed, &s_pppif);
    ESP_LOGI(TAG, "pppapi_pppoe_create returned %p", ppp);

    err_t err = pppapi_connect(ppp, 0);
    ESP_LOGI(TAG, "pppapi_connect returned %d", err);

}
/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    esp_netif_t* netif = arg;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        setup_pppoe(netif);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
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

void app_main(void)
{
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Ethernet driver
    ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_port_cnt));

    if (eth_port_cnt != 1) {
        ESP_LOGE(TAG, "Error: Only one Ethernet instance is supported in this example");
        abort();
    }

    // create the Ethernet netif with almost default config, without DHCP and custom name
    esp_netif_config_t cfg = {};
    esp_netif_inherent_config_t base_cfg = ESP_NETIF_INHERENT_DEFAULT_ETH();
    base_cfg.flags = 0;
    base_cfg.if_key = "pppoe";
    cfg.base = &base_cfg;
    cfg.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH;

    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, eth_netif));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));

}
