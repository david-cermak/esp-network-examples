/***
  This file is part of avahi.

  avahi is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  avahi is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
  Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with avahi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <string.h>
#include "iface.h"
#include "log.h"
#include "avahi-common/malloc.h"
#include "arpa/inet.h"

int avahi_interface_monitor_init_osdep(AvahiInterfaceMonitor *m) {
    AvahiHwInterface *hw;
    AvahiInterface *iface;
    AvahiInterfaceAddress *addr;
    AvahiAddress address;

    assert(m);

    // Create a hardware interface for ESP32's primary network interface
    hw = avahi_hw_interface_new(m, 1); // Use index 1
    if (!hw) {
        avahi_log_error("Failed to create hardware interface");
        return -1;
    }

    // Set basic hardware interface properties
    hw->name = avahi_strdup("WIFI_STA");  // Name for WiFi station interface
    hw->flags_ok = 1;  // Mark interface as usable
    hw->mtu = 1500;    // Standard MTU size

    // Create IPv4 interface
    iface = avahi_interface_new(m, hw, AVAHI_PROTO_INET);
    if (!iface) {
        avahi_log_error("Failed to create IPv4 interface");
        return -1;
    }

    // Add IPv4 address (you'll need to set this to the actual IP)
    address.proto = AVAHI_PROTO_INET;
    inet_pton(AF_INET, "0.0.0.0", &address.data.ipv4.address);  // Will be updated when IP is assigned

    addr = avahi_interface_address_new(m, iface, &address, 24);  // /24 subnet
    if (!addr) {
        avahi_log_error("Failed to create interface address");
        return -1;
    }
    addr->global_scope = 1;

    // Mark interface monitor as complete
    m->list_complete = 1;
    avahi_log_info("Interface monitor initialized with dummy interface");

    // Check if interface is relevant and update RRs
    avahi_interface_check_relevant(iface);
    avahi_interface_update_rrs(iface, 0);

    return 0;
}

void avahi_interface_monitor_free_osdep(AvahiInterfaceMonitor *m) {
    // Nothing special to clean up
}

void avahi_interface_monitor_sync(AvahiInterfaceMonitor *m) {
    // Nothing to sync in this simple implementation
    m->list_complete = 1;
}
