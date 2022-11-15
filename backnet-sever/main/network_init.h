#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"

extern EventGroupHandle_t s_net_event_group;
const static int CONNECTED_BIT = BIT0;

// called after we get IP
void StartBACnet(void);

// init network
void network_init(void);

esp_netif_t* get_netif(void);