#!/usr/bin/env python3
"""
Scapy TCP Server - Manual TCP handshake and HTTP response

Listens for incoming TCP SYN on a specified port, performs handshake, waits for HTTP GET,
and sends a large payload as HTTP response, all using Scapy for full control.

Usage:
    sudo python3 scapy_tcp_server.py --port 3333

NOTE: Block kernel RSTs with:
    iptables -A OUTPUT -p tcp --sport 3333 --tcp-flags RST RST -j DROP
    kill the server: pgrep -f scapy | xargs sudo kill
"""
import argparse
import random
import time
from scapy.all import *

MTU = 1500
DEF_PORT = 3333
RESPONSE_SIZE = 1380

class ScapyTcpServer:
    def __init__(self, port=DEF_PORT, iface='lo'):
        self.port = port
        self.sport = port
        self.seq = None
        self.ack = None
        self.client_ip = None
        self.client_port = None
        self.iface = iface
        self.response_data = self._create_response_data()

    def _create_response_data(self):
        base_text = "SCAPY_TCP_TEST_"
        pattern = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        data = base_text
        while len(data) < RESPONSE_SIZE:
            remaining = RESPONSE_SIZE - len(data)
            if remaining >= len(pattern):
                data += pattern
            else:
                data += pattern[:remaining]
        return data[:RESPONSE_SIZE].encode()

    def _wait_for_syn(self):
        print(f"[SERVER] Waiting for SYN on port {self.port} (iface={self.iface})...")
        def syn_filter(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt[TCP].dport == self.sport
                and pkt[TCP].flags & 0x02 == 0x02
            )
        try:
            pkts = sniff(filter=f"tcp port {self.port}", lfilter=syn_filter, count=1, timeout=60, iface=self.iface)
        except KeyboardInterrupt:
            print("[SERVER] KeyboardInterrupt during SYN wait. Exiting.")
            raise
        if not pkts:
            return None
        pkt = pkts[0]
        print(f"[DEBUG] SYN packet sniffed: {pkt.summary()}")
        self.client_ip = pkt[IP].src
        self.client_port = pkt[TCP].sport
        self.ack = pkt[TCP].seq + 1
        self.seq = random.randrange(0, (2**32) - 1)
        print(f"[SERVER] Got SYN from {self.client_ip}:{self.client_port}")
        print(f"[DEBUG] Initial SEQ={self.seq}, ACK={self.ack}")
        return pkt

    def _send_synack(self):
        ip = IP(dst=self.client_ip)
        synack = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='SA')
        print(f"[SERVER] Sending SYN/ACK (SEQ={self.seq}, ACK={self.ack})")
        send(synack, verbose=0, iface=self.iface)
        print(f"[DEBUG] SYN/ACK sent: {synack.summary()}")

    def _wait_for_ack(self):
        print(f"[SERVER] Waiting for ACK from {self.client_ip}:{self.client_port}...")
        def ack_filter(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt[IP].src == self.client_ip
                and pkt[TCP].sport == self.client_port
                and pkt[TCP].dport == self.sport
                and pkt[TCP].flags & 0x10 == 0x10
                # and pkt[TCP].ack == self.seq + 1
            )
        try:
            pkts = sniff(filter=f"tcp and src host {self.client_ip} and src port {self.client_port}", lfilter=ack_filter, count=1, timeout=60, iface=self.iface)
        except KeyboardInterrupt:
            print("[SERVER] KeyboardInterrupt during ACK wait. Exiting.")
            raise
        if not pkts:
            print("[SERVER] ACK not received, aborting.")
            return False
        print(f"[DEBUG] ACK packet sniffed: {pkts[0].summary()}")
        print(f"[SERVER] Got ACK, handshake complete.")
        self.seq += 1
        print(f"[DEBUG] Updated SEQ={self.seq}, ACK={self.ack}")
        return True

    def _wait_for_get(self):
        print(f"[SERVER] Waiting for HTTP GET from {self.client_ip}:{self.client_port}...")
        def get_filter(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt.haslayer(Raw)
                and pkt[IP].src == self.client_ip
                and pkt[TCP].sport == self.client_port
                and pkt[TCP].dport == self.sport
                and b"GET" in pkt[Raw].load
            )
        try:
            print(f"[DEBUG] Waiting for HTTP GET from {self.client_ip}:{self.client_port}...")
            pkts = sniff(filter=f"tcp and src host {self.client_ip} and src port {self.client_port} and dst port {self.sport}", lfilter=get_filter, count=1, timeout=5, iface=self.iface)
            print(f"[DEBUG] Waiting for HTTP GET from {self.client_ip}:{self.client_port}... {pkts}")
        except KeyboardInterrupt:
            print("[SERVER] KeyboardInterrupt during GET wait. Exiting.")
            raise
        if not pkts:
            print("[SERVER] No HTTP GET received, aborting.")
            return None
        print(f"[DEBUG] HTTP GET packet sniffed: {pkts[0].summary()}")
        try:
            print(f"[DEBUG] HTTP GET payload: {pkts[0][Raw].load[:100]!r}")
        except Exception:
            pass
        # print(f"[SERVER] Got HTTP GET request.")
        self.ack = pkts[0][TCP].seq + len(pkts[0][Raw].load)
        # print(f"[DEBUG] Updated ACK={self.ack}")
        return pkts[0]

    def _send_ack(self):
        ip = IP(dst=self.client_ip)
        ack_pkt = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='A')
        print(f"[SERVER] Sending ACK for HTTP GET (SEQ={self.seq}, ACK={self.ack})")
        send(ack_pkt, verbose=0, iface=self.iface)
        print(f"[DEBUG] ACK sent: {ack_pkt.summary()}")

    def _send_http_response(self):
        ip = IP(dst=self.client_ip)
        headers = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(self.response_data)}\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).encode()
        payload = headers + self.response_data
        print(f"[SERVER] Sending HTTP response ({len(payload)} bytes) to {self.client_ip}:{self.client_port}")
        psh = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='PA')/payload
        send(psh, verbose=0, iface=self.iface)
        print(f"[DEBUG] HTTP response sent: {psh.summary()}")
        print(f"[DEBUG] Response SEQ={self.seq}, next SEQ={self.seq + len(payload)}, ACK={self.ack}")
        self.seq += len(payload)

    def _wait_for_fin(self):
        print(f"[SERVER] Waiting for FIN from {self.client_ip}:{self.client_port}...")
        def fin_filter(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt[IP].src == self.client_ip
                and pkt[TCP].sport == self.client_port
                and pkt[TCP].dport == self.sport
                and pkt[TCP].flags & 0x01 == 0x01
            )
        try:
            pkts = sniff(filter=f"tcp and src host {self.client_ip} and src port {self.client_port} and dst port {self.sport}", lfilter=fin_filter, count=1, timeout=60, iface=self.iface)
        except KeyboardInterrupt:
            print("[SERVER] KeyboardInterrupt during FIN wait. Exiting.")
            raise
        if not pkts:
            print("[SERVER] No FIN received, closing.")
            return
        print(f"[DEBUG] FIN packet sniffed: {pkts[0].summary()}")
        self.ack = pkts[0][TCP].seq + 1
        print(f"[SERVER] Got FIN, sending FIN/ACK and closing.")
        ip = IP(dst=self.client_ip)
        finack = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='FA')
        send(finack, verbose=0, iface=self.iface)
        print(f"[DEBUG] FIN/ACK sent: {finack.summary()}")
        print(f"[DEBUG] Final SEQ={self.seq}, ACK={self.ack}")

    def run(self):
        while True:
            self.seq = None
            self.ack = None
            self.client_ip = None
            self.client_port = None
            try:
                syn_pkt = self._wait_for_syn()
                if syn_pkt is None:
                    continue
                self._send_synack()
                if not self._wait_for_ack():
                    continue
                get_pkt = self._wait_for_get()
                if not get_pkt:
                    continue
                self._send_ack()
                self._send_http_response()
                self._wait_for_fin()
                print(f"[SERVER] Session complete. Ready for next connection.\n")
            except KeyboardInterrupt:
                print("[SERVER] Shutting down.")
                break
            except Exception as e:
                print(f"[SERVER] Error: {e}")
                continue

def main():
    parser = argparse.ArgumentParser(description='Scapy TCP Server for HTTP Window Testing')
    parser.add_argument('--port', type=int, default=DEF_PORT, help='Server port')
    parser.add_argument('--iface', type=str, default='lo', help='Network interface to sniff/send on (default: lo)')
    args = parser.parse_args()
    print(f"[SCAPY TCP SERVER] Listening on port {args.port} (iface={args.iface})")
    print(f"[SCAPY TCP SERVER] Make sure to block kernel RSTs with iptables!")
    server = ScapyTcpServer(port=args.port, iface=args.iface)
    server.run()

if __name__ == '__main__':
    main() 