//
// Copyleft  F.Chaxel 2017
//

#include "config.h"
#include "txbuf.h"
#include "client.h"

#include "handlers.h"
#include "datalink.h"
#include "dcc.h"
#include "tsm.h"
// conflict filename address.h with another file in default include paths
#include "../lib/stack/address.h"
#include "bip.h"

#include "device.h"
#include "ai.h"
#include "bo.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// hidden function not in any .h files
extern uint8_t temprature_sens_read();
extern uint32_t hall_sens_read();

// GPIO 5 has a Led on Sparkfun ESP32 board
#define BACNET_LED 5
#define TAG "bacnet-sta"

#define EXAMPLE_ESP_WIFI_SSID      "CONFIG_ESP_WIFI_SSID"
#define EXAMPLE_ESP_WIFI_PASS      "CONFIG_ESP_WIFI_PASSWORD"

uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

/** Static receive buffer, initialized with zeros by the C Library Startup Code. */

uint8_t Rx_Buf[MAX_MPDU + 16 /* Add a little safety margin to the buffer,
                              * so that in the rare case, the message
                              * would be filled up to MAX_MPDU and some
                              * decoding functions would overrun, these
                              * decoding functions will just end up in
                              * a safe field of static zeros. */] = { 0 };

EventGroupHandle_t s_wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/* BACnet handler, stack init, IAm */
void StartBACnet()
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);

    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);

    address_init();
    bip_init(NULL);
    Send_I_Am(&Handler_Transmit_Buffer[0]);
}

/* wifi events handler : start & stop bacnet with an event  */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "retry to connect to the AP");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        bip_cleanup();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        if (xEventGroupGetBits(s_wifi_event_group) != CONNECTED_BIT) {
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            StartBACnet();
        }
    }
}

/* tcpip & wifi station start */
void wifi_init_station(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    xEventGroupWaitBits(s_wifi_event_group,
            CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
}

/* setup gpio & nv flash, call wifi init code */
void setup()
{
    esp_rom_gpio_pad_select_gpio(BACNET_LED);
    gpio_set_direction(BACNET_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(BACNET_LED, 0);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    wifi_init_station();
}

/* Bacnet Task */
void BACnetTask(void *pvParameters)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };
    unsigned timeout = 1;

    // Init Bacnet objets dictionnary
    Device_Init(NULL);
    Device_Set_Object_Instance_Number(12);

    setup();

    uint32_t tickcount = xTaskGetTickCount();

    for (;;) {
        vTaskDelay(
            10 / portTICK_PERIOD_MS); // could be remove to speed the code

        // do nothing if not connected to wifi
        xEventGroupWaitBits(
            s_wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        {
            uint32_t newtick = xTaskGetTickCount();

            // one second elapse at least (maybe much more if Wifi was
            // deconnected for a long)
            if ((newtick < tickcount) ||
                ((newtick - tickcount) >= configTICK_RATE_HZ)) {
                tickcount = newtick;
                dcc_timer_seconds(1);
                bvlc_maintenance_timer(1);
                handler_cov_timer_seconds(1);
                tsm_timer_milliseconds(1000);

                // Read analog values from internal sensors
                Analog_Input_Present_Value_Set(0, temprature_sens_read());
                Analog_Input_Present_Value_Set(1, hall_sens_read());
            }

            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);

                if (Binary_Output_Present_Value(0) == BINARY_ACTIVE)
                    gpio_set_level(BACNET_LED, 1);
                else
                    gpio_set_level(BACNET_LED, 0);
            }

            handler_cov_task();
        }
    }
}
/* Entry point */
void app_main()
{
    // Cannot run BACnet code here, the default stack size is to small : 4096
    // byte
    xTaskCreate(BACnetTask, /* Function to implement the task */
        "BACnetTask", /* Name of the task */
        10000, /* Stack size in words */
        NULL, /* Task input parameter */
        20, /* Priority of the task */
        NULL); /* Task handle. */
}
