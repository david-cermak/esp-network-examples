/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "more_netifs";
static EventGroupHandle_t event_group = NULL;
static const int STA_JOINED = BIT0;
static const int GOT_IP = BIT1;
static esp_netif_t* ap;
static esp_netif_t* sta;
void* esp_netif_get_netif_impl(esp_netif_t *esp_netif);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WIFI event! %d", event_id);
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(event_group, STA_JOINED);
        esp_netif_dhcpc_stop(ap);
        esp_netif_ip_info_t ip_info = {
                .ip = { .addr = ESP_IP4TOADDR( 0, 0, 0, 0) },
                .gw = { .addr = ESP_IP4TOADDR( 0, 0, 0, 0) },
                .netmask = { .addr = ESP_IP4TOADDR( 0, 0, 0, 0) },
        };
        esp_netif_set_ip_info(ap, &ip_info);
        esp_netif_dhcpc_start(ap);
        esp_netif_action_connected(ap, event_base,event_id,event_data);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
//        esp_netif_action_disconnected(ap, 0,0,0);

    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    esp_netif_t* netif = event->esp_netif;
    ESP_LOGI(TAG, "IP event! %d, netif: %p", event_id, netif);
    if (event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(event_group, GOT_IP);
        ESP_LOGW(TAG, "Got IPv4 from interface: \"%s\" (key=\"%s\") ", esp_netif_get_desc(netif), esp_netif_get_ifkey(netif));
        if (netif == ap) {
            ESP_LOGI(TAG, "Our custom AP got an IP -> setting the netif up");
            esp_netif_action_connected(ap, event_base,event_id,event_data);
        }
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));

    } else if (event_id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGE(TAG, "IP LOST event!");
        if (netif == ap) {
            ESP_LOGI(TAG, "Our custom AP lost its IP -> setting the netif down");
            esp_netif_action_disconnected(ap, event_base,event_id,event_data);
            netif_set_link_down(esp_netif_get_netif_impl(ap));
        }
    }
}


void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    esp_netif_t* ap = esp_netif_create_default_wifi_ap();
    sta = esp_netif_create_default_wifi_sta();
    ESP_LOGI(TAG, "Created STA network interface: %p", sta);

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
//    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_AP();
//    esp_netif_config.ip_info = &ip_info;
    esp_netif_config.if_desc = "wifi access point with dhcp client";
    esp_netif_config.if_key = "AP_2";
    esp_netif_config.route_prio = 101;
    ap = esp_netif_create_wifi(WIFI_IF_AP, &esp_netif_config);
    ESP_LOGI(TAG, "Created AP network interface: %p", ap);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    wifi_config_t wifi_config_sta = {
            .sta = {
                    .ssid = "CermakowiFi",
                    .password = "MojeNovaUfka",
//                    .scan_method = EXAMPLE_WIFI_SCAN_METHOD,
//                    .sort_method = EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD,
//                    .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
//                    .threshold.authmode = EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGI(TAG, "wifi_init finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if ((bool)evt->user_data &&
                !esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOG_BUFFER_HEXDUMP(TAG, evt->data, evt->data_len, ESP_LOG_INFO);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default: break;
    }
    return ESP_OK;
}
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    event_group = xEventGroupCreate();

    wifi_init();

    xEventGroupWaitBits(event_group, /*STA_JOINED | */GOT_IP, pdTRUE, pdTRUE, portMAX_DELAY);
    ESP_LOGW(TAG, "Both STA and AP are up and running!");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_http_client_config_t config = {
            .event_handler = http_event_handler,
            .url = "http://httpbin.org/get",
    };
    ESP_LOGW(TAG, "Starting http client!");

    while (1)
    {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(ap, &ip_info);
        ESP_LOGW(TAG, "IP info: %x", ip_info.ip.addr);
        if (ip_info.ip.addr == 0) {
            netif_set_link_down(esp_netif_get_netif_impl(ap));
            netif_set_default(esp_netif_get_netif_impl(sta));
//            netif_set_link_up(esp_netif_get_netif_impl(ap));
        }
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            uint64_t content_length = esp_http_client_get_content_length(client);
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                     esp_http_client_get_status_code(client), content_length);
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
