# Minimal example demonstrating DHCP options VENDOR_CLASS_IDENTIFIER and VENDOR_SPECIFIC_INFORMATION

```
I (354) dhcp-vendor-spec: [APP] Startup..
I (354) dhcp-vendor-spec: [APP] Free memory: 297500 bytes
I (354) dhcp-vendor-spec: [APP] IDF version: v5.4-683-g0ba53566fad-dirty
I (434) esp_eth.netif.netif_glue: ethernet attached to netif
dhcp_coarse_tmr()
dhcp_coarse_tmr()
I (2834) ethernet_connect: Waiting for IP(s).
dhcp_start(netif=0x3ffb92a4) en1
dhcp_start(): mallocing new DHCP client
dhcp_start(): allocated dhcp
dhcp_start(): starting DHCP configuration
dhcp_discover()
dhcp_set_state(): old state: /0x00 -> new state: /0x06
transaction id xid(5be42480)
dhcp_discover: making request
dhcp_discover: sendto(DISCOVER, IP_ADDR_BROADCAST, LWIP_IANA_PORT_DHCP_SERVER)
dhcp_discover: deleting()
dhcp_discover: SELECTING
dhcp_discover(): set request timeout 500 msecs
I (2864) ethernet_connect: Ethernet Link Up
dhcp_release_and_stop()
dhcp_set_state(): old state: /0x06 -> new state: /0x00
I (2874) dhcp-vendor-spec: DHCP client stopped: ESP_OK
I (2884) dhcp-vendor-spec: DHCP client option set: ESP_OK
dhcp_start(netif=0x3ffb92a4) en1
dhcp_start(): restarting DHCP configuration
dhcp_start(): starting DHCP configuration
dhcp_discover()
dhcp_set_state(): old state: /0x00 -> new state: /0x06
transaction id xid(9630ab57)
dhcp_discover: making request
dhcp_discover: sendto(DISCOVER, IP_ADDR_BROADCAST, LWIP_IANA_PORT_DHCP_SERVER)
dhcp_discover: deleting()
dhcp_discover: SELECTING
dhcp_discover(): set request timeout 500 msecs
I (2924) dhcp-vendor-spec: DHCP client started: ESP_OK
dhcp_recv(pbuf = 0x3ffbd000) from DHCP server 192.168.5.1 port 67
pbuf->len = 300
pbuf->tot_len = 300
skipping option 43 in options
searching DHCP_OPTION_MESSAGE_TYPE
DHCP_OFFER received in DHCP_STATE_SELECTING state
dhcp_handle_offer(netif=0x3ffb92a4) en1
dhcp_handle_offer(): server 0x0105a8c0
dhcp_handle_offer(): offer for 0x0805a8c0
dhcp_select(netif=0x3ffb92a4) en1
dhcp_set_state(): old state: /0x06 -> new state: /0x01
transaction id xid(9630ab57)
dhcp_select: REQUESTING
dhcp_select(): set request timeout 500 msecs
dhcp_recv(pbuf = 0x3ffbd034) from DHCP server 192.168.5.1 port 67
pbuf->len = 300
pbuf->tot_len = 300
skipping option 43 in options
searching DHCP_OPTION_MESSAGE_TYPE
DHCP_ACK received
dhcp_check(netif=0x3ffb92a4) en
dhcp_set_state(): old state: /0x01 -> new state: /0x08
dhcp_coarse_tmr()
dhcp_bind(netif=0x3ffb92a4) en1
dhcp_bind(): t0 renewal timer 548 secs
dhcp_bind(): set request timeout 548000 msecs
dhcp_bind(): t1 renewal timer 274 secs
dhcp_bind(): set request timeout 274000 msecs
dhcp_bind(): t2 rebind timer 479 secs
dhcp_bind(): set request timeout 479000 msecs
dhcp_bind(): IP: 0x0805a8c0 SN: 0x00ffffff GW: 0x0105a8c0
dhcp_set_state(): old state: /0x08 -> new state: /0x0a
I (4024) esp_netif_handlers: example_netif_eth ip: 192.168.5.8, mask: 255.255.255.0, gw: 192.168.5.1
I (4034) ethernet_connect: Got IPv4 event: Interface "example_netif_eth" address: 192.168.5.8
dhcp_coarse_tmr()
I (4384) ethernet_connect: Got IPv6 event: Interface "example_netif_eth" address: fe80:0000:0000:0000:xxxx:xxxx:xxxx:xxxx, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4384) example_common: Connected to example_netif_eth
I (4384) example_common: - IPv4 address: 192.168.5.8,
I (4394) example_common: - IPv6 address: fe80:0000:0000:0000:xxxx:xxxx:xxxx:xxxx, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4404) dhcp-vendor-spec: DHCP client option get: ESP_OK
I (4404) dhcp-vendor-spec: vendor string hallo test 1234
I (4414) main_task: Returned from app_main()
```