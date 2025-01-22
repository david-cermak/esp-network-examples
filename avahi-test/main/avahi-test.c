#include <stdio.h>
#include <avahi-common/error.h>
#include "sys/poll.h"
#include "core.h"
#include "log.h"
#include "avahi-common/simple-watch.h"
#include "assert.h"
#include <avahi-core/lookup.h>
#include <stdlib.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "sys/utsname.h"
#include <string.h>

// AvahiServer *avahi_server = NULL;
// AvahiSimplePoll *simple_poll_api = NULL;
// AvahiServerConfig server_config = {};


void avahi_init_i18n(void)
{
}

int uname (struct utsname *__name)
{
    strcpy(__name->sysname, "ESP32");
    strcpy(__name->nodename, "esp32");
    strcpy(__name->release, "1.0");
    strcpy(__name->version, "1.0");
    strcpy(__name->machine, "esp32");
    return 0;
}

// static void server_callback(AvahiServer *s, AvahiServerState state, void *userdata) {
//     assert(s);

//     /* This function is possibly called before the global variable
//      * avahi_server has been set, therefore we do it explicitly */

//     avahi_server = s;

// #ifdef HAVE_DBUS
//     if (c->enable_dbus && state != AVAHI_SERVER_INVALID && state != AVAHI_SERVER_FAILURE)
//         dbus_protocol_server_state_changed(state);
// #endif

//     switch (state) {
//         case AVAHI_SERVER_RUNNING:
//             avahi_log_info("Server startup complete. Host name is Local service cookie is");
//             break;

//         case AVAHI_SERVER_COLLISION: {
//             avahi_log_info("AVAHI_SERVER_COLLISION");
//             break;
//         }

//         case AVAHI_SERVER_FAILURE:

//             avahi_log_error("Server error: %s", avahi_strerror(avahi_server_errno(s)));
//             avahi_simple_poll_quit(simple_poll_api);
//             break;

//         case AVAHI_SERVER_REGISTERING:
//             avahi_log_info("AVAHI_SERVER_REGISTERING");
//             break;

//         case AVAHI_SERVER_INVALID:
//             avahi_log_info("AVAHI_SERVER_INVALID");
//             break;

//     }
// }
#define DOMAIN NULL
#define SERVICE_TYPE "_http._tcp"

static AvahiSServiceBrowser *service_browser1 = NULL, *service_browser2 = NULL;
static const AvahiPoll * poll_api = NULL;
static AvahiServer *server = NULL;
static AvahiSimplePoll *simple_poll;

static const char *browser_event_to_string(AvahiBrowserEvent event) {
    switch (event) {
        case AVAHI_BROWSER_NEW : return "NEW";
        case AVAHI_BROWSER_REMOVE : return "REMOVE";
        case AVAHI_BROWSER_CACHE_EXHAUSTED : return "CACHE_EXHAUSTED";
        case AVAHI_BROWSER_ALL_FOR_NOW : return "ALL_FOR_NOW";
        case AVAHI_BROWSER_FAILURE : return "FAILURE";
    }

    abort();
}

static void sb_callback(
    AvahiSServiceBrowser *b,
    AvahiIfIndex iface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *service_type,
    const char *domain,
    AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void* userdata) {
        // ESP_LOGI("AVAHI", "check");
        if (name) {
            ESP_LOGI("AVAHI", "SB%i: (%i.%s) <%s> as <%s> in <%s> [%s] cached=%i", b == service_browser1 ? 1 : 2, iface, avahi_proto_to_string(protocol), name, service_type, domain, browser_event_to_string(event), !!(flags & AVAHI_LOOKUP_RESULT_CACHED));
        }
        avahi_log_debug("NAME: %p", name);
    // avahi_log_debug("SB%i: (%i.%s) <%s> as <%s> in <%s> [%s] cached=%i", b == service_browser1 ? 1 : 2, iface, avahi_proto_to_string(protocol), name, service_type, domain, browser_event_to_string(event), !!(flags & AVAHI_LOOKUP_RESULT_CACHED));
}

static void create_second_service_browser(AvahiTimeout *timeout, AVAHI_GCC_UNUSED void* userdata) {

    service_browser2 = avahi_s_service_browser_new(server, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, SERVICE_TYPE, DOMAIN, 0, sb_callback, NULL);
    assert(service_browser2);

    poll_api->timeout_free(timeout);
}

static void quit(AVAHI_GCC_UNUSED AvahiTimeout *timeout, AVAHI_GCC_UNUSED void *userdata) {
    avahi_simple_poll_quit(simple_poll);
}


void app_main(void)
{
#ifndef CONFIG_IDF_TARGET_LINUX
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
#endif
    struct timeval tv;
    AvahiServerConfig config;

    simple_poll = avahi_simple_poll_new();
    assert(simple_poll);

    poll_api = avahi_simple_poll_get(simple_poll);
    assert(poll_api);

    avahi_server_config_init(&config);
    config.publish_hinfo = 0;
    config.publish_addresses = 0;
    config.publish_workstation = 0;
    config.publish_domain = 0;

    avahi_address_parse("192.168.32.3", AVAHI_PROTO_UNSPEC, &config.wide_area_servers[0]);
    config.n_wide_area_servers = 1;
    config.enable_wide_area = 1;
    config.use_ipv4 = 1;
    config.use_ipv6 = 0;
    server = avahi_server_new(poll_api, &config, NULL, NULL, NULL);
    assert(server);
    avahi_server_config_free(&config);

//    esp_netif_t* netif = EXAMPLE_INTERFACE;
//    int ifindex = esp_netif_get_netif_impl_index(netif);
    
//    esp_netif_ip_info_t ip_info;
//    esp_netif_get_ip_info(netif, &ip_info);
//    printf("Interface index: %d\n", ifindex);
//    printf("IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
//    printf("Netmask: " IPSTR "\n", IP2STR(&ip_info.netmask));
//    printf("Gateway: " IPSTR "\n", IP2STR(&ip_info.gw));
    // Also check interface flags
//    uint8_t mac[6];
//    esp_netif_get_mac(netif, mac);
//    printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
//           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    service_browser1 = avahi_s_service_browser_new(
        server,
        AVAHI_IF_UNSPEC,
//        ifindex,
        AVAHI_PROTO_INET,
        SERVICE_TYPE, 
        DOMAIN, 
        AVAHI_LOOKUP_USE_MULTICAST, // Explicitly request multicast
        sb_callback, 
        NULL
    );
    assert(service_browser1);

    // poll_api->timeout_new(poll_api, avahi_elapse_time(&tv, 10000, 0), create_second_service_browser, NULL);

    poll_api->timeout_new(poll_api, avahi_elapse_time(&tv, 60000, 0), quit, NULL);


    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Shorter delay
        if (avahi_simple_poll_iterate(simple_poll, 0) != 0) // Non-blocking poll
            break;
    }

    avahi_server_free(server);
    avahi_simple_poll_free(simple_poll);

    // const AvahiPoll *poll_api = NULL;
    // poll_api = avahi_simple_poll_get(simple_poll_api);
    // int error;
    // if (!(avahi_server = avahi_server_new(poll_api, &server_config, server_callback, &server_config, &error))) {
    //     avahi_log_error("Failed to create server: %s", avahi_strerror(error));
    //     goto finish;
    // }

//    update_wide_area_servers();
//    update_browse_domains();
//finish:
//    avahi_log_error("Failed to create server");
}
