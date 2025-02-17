# Setup PPPoE server on linux

## Install and run

Install PPPoE
```
sudo apt update
sudo apt install pppoe pppoeconf ppp
```

### Use no authentication

### Configure
```
# Create /etc/ppp/pppoe-server-options
debug
logfile /var/log/pppoe-server.log
lcp-echo-interval 10
lcp-echo-failure 3
ms-dns 8.8.8.8
netmask 255.255.255.0
noauth
mtu 1492
mru 1492
```

### Run

```
sudo pppoe-server -I <eth-netif> -L 192.168.1.1 -R 192.168.1.100 -N 10
```

## Run the project:

```
I (354) main_task: Started on CPU0
I (364) main_task: Calling app_main()
I (404) esp_eth.netif.netif_glue: xx:xx:xx:xx:xx:xx
I (404) esp_eth.netif.netif_glue: ethernet attached to netif
I (3104) ethernet_init: Ethernet(IP101[23,18]) Started
I (3104) pppoe_example: Ethernet Started
I (3104) ethernet_init: Ethernet(IP101[23,18]) Link Up
I (3104) ethernet_init: Ethernet(IP101[23,18]) HW Addr xx:xx:xx:xx:xx:xx
I (3104) main_task: Returned from app_main()
ppp phase changed[2]: phase=0
I (3114) pppoe_example: pppapi_pppoe_create returned 0x3ffbc4f4
ppp_connect[2]: holdoff=0
ppp phase changed[2]: phase=3
pppoe: en1 (8863) state=1, session=0x0 output -> ff:ff:ff:ff:ff:ff, len=32
I (3134) pppoe_example: pppapi_connect returned 0
pppoe: en1 (8863) state=2, session=0x0 output -> xx:xx:xx:xx:xx:xx, len=56
I (3134) pppoe_example: Ethernet Link Up
pppoe: en1: session 0x3 connected
I (3144) pppoe_example: Ethernet HW Addr xx:xx:xx:xx:xx:xx
ppp_start[2]
ppp phase changed[2]: phase=6
ppp_send_config[2]
ppp_recv_config[2]
sent [LCP ConfReq id=0x1 <mru 1492> <magic 0xb67d01ae>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=36
ppp_start[2]: finished
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=16
rcvd [LCP ConfReq id=0x1 <mru 1492> <magic 0x68bbf1ad>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [LCP ConfAck id=0x1 <mru 1492> <magic 0x68bbf1ad>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=36
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=16
rcvd [LCP ConfReq id=0x1 <mru 1492> <magic 0x68bbf1ad>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [LCP ConfAck id=0x1 <mru 1492> <magic 0x68bbf1ad>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=36
sent [LCP ConfReq id=0x1 <mru 1492> <magic 0xb67d01ae>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=36
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=16
rcvd [LCP ConfAck id=0x1 <mru 1492> <magic 0xb67d01ae>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
ppp_netif_set_mtu[2]: mtu=1492
ppp_send_config[2]
ppp_recv_config[2]
ppp phase changed[2]: phase=9
sent [IPCP ConfReq id=0x1 <addr 0.0.0.0>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=32
sent [IPV6CP ConfReq id=0x1 <addr fe80::xxxx:xxxx:xxxx:xxxx>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=36
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=10
rcvd [LCP EchoReq id=0x0 magic=0x68bbf1ad] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [LCP EchoRep id=0x0 magic=0xb67d01ae]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=30
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=17
Unsupported protocol 'Compression Control Protocol' (0x80fd) received
ppp_input[2]: Dropping (pbuf_add_header failed)
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=18
rcvd [IPCP ConfReq id=0x1 <compress VJ 0f 01> <addr 192.168.1.1>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [IPCP ConfRej id=0x1 <compress VJ 0f 01>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=32
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=12
rcvd [IPCP ConfNak id=0x1 <addr 192.168.1.102>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [IPCP ConfReq id=0x2 <addr 192.168.1.102>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=32
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=22
rcvd [IPCP ConfReq id=0x2 <addr 192.168.1.1>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sent [IPCP ConfAck id=0x2 <addr 192.168.1.1>]
pppoe: en1 (8864) state=3, session=0x3 output -> xx:xx:xx:xx:xx:xx, len=32
pppoe_data_input: en1: pkthdr.len=40, pppoe.len=12
rcvd [IPCP ConfAck id=0x2 <addr 192.168.1.102>] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
sifvjcomp[2]: VJ compress enable=0 slot=0 max slot=0
sifup[2]: err_code=0
I (9394) pppoe_example: Connected with IP=192.168.1.102
local  IP address 192.168.1.102
remote IP address 192.168.1.1
ppp phase changed[2]: phase=10

```