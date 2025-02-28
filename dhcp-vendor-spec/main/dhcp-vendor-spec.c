#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_eth.h"

static const char *TAG = "dhcp-vendor-spec";

static void on_network_start(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    const char *vendor_class_id = "Testing";
    esp_err_t err = esp_netif_dhcpc_stop(esp_netif);
    ESP_LOGI(TAG, "DHCP client stopped: %s", esp_err_to_name(err));
    err = esp_netif_dhcpc_option(esp_netif, ESP_NETIF_OP_SET, ESP_NETIF_VENDOR_CLASS_IDENTIFIER, (void*)vendor_class_id, strlen(vendor_class_id));
    ESP_LOGI(TAG, "DHCP client option set: %s", esp_err_to_name(err));
    err = esp_netif_dhcpc_start(esp_netif);
    ESP_LOGI(TAG, "DHCP client started: %s", esp_err_to_name(err));
}

static void register_events(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, on_network_start, netif));
    } else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_START) {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, on_network_start, netif));
    }
}


void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Need to register the network event "after" the driver started,
     * normally we'd just call it after eth/wifi started, but here we're using `example_connect()` call
     * which abstracts all driver/init functionality, so we create an event for registration */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_START, &register_events, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &register_events, NULL));

    ESP_ERROR_CHECK(example_connect());
    char vendor_buf[128] = {0};
    esp_err_t err = esp_netif_dhcpc_option(EXAMPLE_INTERFACE, ESP_NETIF_OP_GET, ESP_NETIF_VENDOR_SPECIFIC_INFO, &vendor_buf[0], sizeof(vendor_buf)-1);
    ESP_LOGI(TAG, "DHCP client option get: %s", esp_err_to_name(err));
    ESP_LOGI(TAG, "vendor string %s", &vendor_buf[0]);
    ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_START, &register_events));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_START, &register_events));
}
