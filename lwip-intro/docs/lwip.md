---
marp: true
theme: espressif
paginate: true
---

# lwIP for IDF developers

*Everything You Always Wanted to Know About `lwip`...*

* lwIP Overview
* IDF port layer
* lwIP vs esp-lwip
* Tips and Tweaks
* How we debug/test lwip
* Future considerations

---

![width:30cm height:15cm](lwip_intro.png)

---

# Demo: TCP connection

*TCP socket example*

```
sudo tshark -i lo/tap0 -f "tcp port 3333"
nc -l 3333
```

* netcat client `nc localhost 3333`
* default socket client
* lwip socket client

---

# lwIP overview

* [Supported protocols and features](https://github.com/espressif/esp-lwip/blob/76303df2386902e0d7873be4217f1d9d1b50f982/README#L14-L35)
* Features and configs used by IDF
* Layering: `netif` -> `IP` -> `UDP/TCP` -> `apps`
* Structures: pbufs, memp, pcb
* threads, ports, core-locking
---

# lwIP features -- Used in IDF

  * IP (Internet Protocol, IPv4 and IPv6), ICMP, IGMP, TCP/UDP
  * DHCP, AutoIP, DNS (mDNS)
  * MLD (Multicast listener discovery for IPv6), ND (Neighbor discovery and stateless address autoconfiguration for IPv6), Stateless DHCPv6
  * ~raw/native API for enhanced performance~
  * ~Optional~ Berkeley-like socket API
  * ~TLS: optional layered TCP ("altcp") for nearly transparent TLS for any TCP-based protocol (ported to mbedTLS) (see changelog for more info)~
  * PPPoS and ~PPPoE~ (Point-to-point protocol over Serial/Ethernet)
  * ~6LoWPAN (via IEEE 802.15.4, BLE or ZEP)~
---

# lwIP structures

* Packet buffers: pbufs
  - support of chaining, rewinding headers
  - types: (`PBUF_RAM`, `PBUF_ROM`, `PBUF_REF`, `PBUF_POOL`)
* Memory pools (`LWIP_MEMPOOL`, used in PPPoS)
* Protocol control block (`tcp_pcb`, `udp_pcb`, `ip_pcb`, `raw_pcb`)
  - e.g. [common/IP related PCB members](https://github.com/espressif/esp-lwip/blob/76303df2386902e0d7873be4217f1d9d1b50f982/src/include/lwip/ip.h#L72-L89)

---
# lwIP contrib

## ports, addons, examples, tests

* [FreeRTOS port](https://github.com/lwip-tcpip/lwip/tree/master/contrib/ports/freertos)
* [Addons](https://github.com/lwip-tcpip/lwip/tree/master/contrib/addons)

---

# Is lwIP light-weight?

* numbers for ESP32 (flash, heap, static)
  - | IPv4 | dual-stack | (IPv6)
* Configurable
* IDF config

---

# How light-weight lwIP is (demo)?

## Demo (socket server, mem-consumption)

```
valgrind --tool=massif --time-unit=ms  --stacks=yes ./example_app
```
![width:30cm height:15cm](mem_consumption.png)

---

```
--------------------------------------------------------------------------------
Command:            ./example_app
Massif arguments:   --threshold=0.1 --time-unit=ms --stacks=yes
ms_print arguments: massif.out.156121
--------------------------------------------------------------------------------


    MB
1.431^                                                                     :::
     |                                                                 @@@#:::
     |                                                            @@@@@@@@#:::
     |                                                         @@@@@@@@@@@#:::
     |                                                    @@@@@@@@@@@@@@@@#:::
     |                                                @@@@@@@@ @@@@@@@@@@@#:::
     |                                             @::@ @@@@@@ @@@@@@@@@@@#:::
     |                                         @@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                                     @::@@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                                  @@@@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                              ::::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                           @@@: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                       :::@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                    @@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |                 @@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |              @@@@@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |           :@@@@@@@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |        @@@:@ @@@@@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |      ::@@@:@ @@@@@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
     |    ::::@@@:@ @@@@@@@@@: :@@ @: ::@ @@: @@@::@::@ @@@@@@ @@@@@@@@@@@#:::
   0 +----------------------------------------------------------------------->s
     0                                                                   24.96

Number of snapshots: 81
 Detailed snapshots: [7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 23, 24, 25, 29, 30, 31, 33, 34, 3
5, 38, 41, 42, 43, 44, 45, 46, 47, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76 (peak)]

--------------------------------------------------------------------------------
  n       time(ms)         total(B)   useful-heap(B) extra-heap(B)    stacks(B)
--------------------------------------------------------------------------------
  0              0                0                0             0            0
  1            375            5,488                0             0        5,488
  2            793           42,784           39,688         1,368        1,728
  3          1,040           68,304           64,304         2,184        1,816
  4          1,468          110,128          104,848         3,528        1,752
  5          2,082          160,960          154,080         5,160        1,720
  6          2,444          193,952          185,936         6,216        1,800
  7          2,933          234,096          224,648         7,480        1,968
95.96% (224,648B) (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
->70.45% (164,920B) 0x16B54B: sys_mbox_new (sys_arch.c:249)
| ->70.00% (163,856B) 0x144A24: netconn_alloc (api_msg.c:770)
| | ->70.00% (163,856B) 0x141BF6: netconn_new_with_proto_and_callback (api_lib.c:155)
| |   ->70.00% (163,856B) 0x14AF9B: lwip_socket (sockets.c:1720)
| |     ->70.00% (163,856B) 0x10B986: tcp_bind_test (socket_examples.c:103)
| |       ->70.00% (163,856B) 0x10F72D: main (socket_examples.c:941)
| |         
| ->00.45% (1,064B) in 1+ places, all below ms_print's threshold (01.00%)
| 
->25.22% (59,040B) 0x16BC75: sys_sem_new_internal (sys_arch.c:450)
| ->06.32% (14,784B) 0x16B582: sys_mbox_new (sys_arch.c:254)
| | ->06.27% (14,688B) 0x144A24: netconn_alloc (api_msg.c:770)
| | | ->06.27% (14,688B) 0x141BF6: netconn_new_with_proto_and_callback (api_lib.c:155)
| | |   ->06.27% (14,688B) 0x14AF9B: lwip_socket (sockets.c:1720)
| | |     ->06.27% (14,688B) 0x10B986: tcp_bind_test (socket_examples.c:103)
| | |       ->06.27% (14,688B) 0x10F72D: main (socket_examples.c:941)
| | |         
| | ->00.04% (96B) in 1+ places, all below ms_print's threshold (01.00%)
| | 
```

# IDF port

- Used/supported API
- FreeRTOS
    - msg-api
    - TLS
    - core-locking
- netif (ethernet, wifi, thread)
- esp-netif

---

#  lwIP and esp-lwip

* Adjust various parameters
* api-msg (close from another thread)
* kill pcb in wait state(s)
* bugfixes
* sys-timers on demand (IGMP, )
* NAPT
* DNS fallback server
* IPv6 zoning

---

#  lwIP and esp-lwip

* show statistics
## Demo

* close/shutdown 
* connect vs. read (select, non-block

---

# Tips and Tricks

* config options to reduce memory (dualstack, buffers)
* TCP adjustments
* LWIP hooks

---

# How we debug/test lwip

* on host 
    - unit tests (check)
    - example_apps
    - fuzzing
* on target -- `IT_*`
* debug prints
* Demo: debugging on host

---

# Future consideration

* Cleanup patches
* Converge to upstream
* Focus on host tests
* Evaluate, measure, test

---

# Takeaways

* DNS servers are global
* No deinit
* timeouts (connect, NULL-tv)

