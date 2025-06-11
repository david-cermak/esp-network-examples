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
import signal
import sys
import time
import threading
from scapy.all import *

MTU = 1500
DEF_PORT = 3333
RESPONSE_SIZE = 1300  # Reduced to avoid IP fragmentation with HTTP headers

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
        self.running = True
        self.global_sniffer_thread = None
        self.global_packets = []
        self.global_packets_lock = threading.Lock()
        
        # Set up signal handlers for graceful shutdown
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
    
    def _signal_handler(self, signum, frame):
        print(f"\n[SERVER] Received signal {signum}, shutting down gracefully...")
        self.running = False
        if self.global_sniffer_thread:
            self.global_sniffer_thread.join(timeout=1)

    def _global_packet_callback(self, pkt):
        """Global packet callback to monitor all TCP traffic"""
        try:
            if pkt.haslayer(TCP) and pkt.haslayer(IP):
                timestamp = time.time()
                src_ip = pkt[IP].src
                dst_ip = pkt[IP].dst
                src_port = pkt[TCP].sport
                dst_port = pkt[TCP].dport
                flags = pkt[TCP].flags
                seq = pkt[TCP].seq
                ack = pkt[TCP].ack
                
                # Only log packets related to our port
                if src_port == self.sport or dst_port == self.sport:
                    flags_str = ""
                    if flags & 0x01: flags_str += "F"
                    if flags & 0x02: flags_str += "S"
                    if flags & 0x04: flags_str += "R"
                    if flags & 0x08: flags_str += "P"
                    if flags & 0x10: flags_str += "A"
                    if flags & 0x20: flags_str += "U"
                    
                    payload_info = ""
                    if pkt.haslayer(Raw):
                        payload = pkt[Raw].load
                        if b"GET" in payload:
                            payload_info = f" [HTTP GET: {payload[:50]!r}]"
                        elif b"HTTP" in payload:
                            payload_info = f" [HTTP RESP: {len(payload)} bytes]"
                        else:
                            payload_info = f" [DATA: {len(payload)} bytes]"
                    
                    print(f"[GLOBAL] {timestamp:.6f} {src_ip}:{src_port} -> {dst_ip}:{dst_port} [{flags_str}] seq={seq} ack={ack}{payload_info}")
                    
                    # Store packet for analysis
                    with self.global_packets_lock:
                        self.global_packets.append({
                            'timestamp': timestamp,
                            'pkt': pkt,
                            'summary': pkt.summary()
                        })
                        # Keep only last 100 packets
                        if len(self.global_packets) > 100:
                            self.global_packets.pop(0)
        except Exception as e:
            print(f"[GLOBAL] Error in packet callback: {e}")

    def _start_global_sniffer(self):
        """Start global packet sniffer in background thread"""
        def sniffer_thread():
            print("[GLOBAL] Starting global packet sniffer...")
            try:
                sniff(filter=f"tcp port {self.sport}", prn=self._global_packet_callback, iface=self.iface, stop_filter=lambda x: not self.running)
            except Exception as e:
                print(f"[GLOBAL] Global sniffer error: {e}")
            print("[GLOBAL] Global packet sniffer stopped.")
        
        self.global_sniffer_thread = threading.Thread(target=sniffer_thread, daemon=True)
        self.global_sniffer_thread.start()
        time.sleep(0.5)  # Give sniffer time to start

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
        
        start_time = time.time()
        print(f"[DEBUG] {start_time:.6f} Starting to look for SYN packet")
        
        # Check if we already have a SYN packet in our global buffer
        with self.global_packets_lock:
            for packet_info in reversed(self.global_packets):  # Check most recent first
                pkt = packet_info['pkt']
                if (pkt.haslayer(TCP) and 
                    pkt[TCP].dport == self.sport and 
                    pkt[TCP].flags & 0x02 == 0x02):  # SYN flag
                    print(f"[DEBUG] Found SYN in global buffer from {packet_info['timestamp']:.6f}")
                    self.client_ip = pkt[IP].src
                    self.client_port = pkt[TCP].sport
                    self.ack = pkt[TCP].seq + 1
                    self.seq = random.randrange(0, (2**32) - 1)
                    print(f"[SERVER] Got SYN from {self.client_ip}:{self.client_port}")
                    print(f"[DEBUG] Initial SEQ={self.seq}, ACK={self.ack}")
                    return pkt
        
        # If not found in buffer, wait for new packets
        print(f"[DEBUG] SYN not in buffer, waiting for new packets...")
        while self.running:
            time.sleep(0.1)  # Check every 100ms
            
            with self.global_packets_lock:
                for packet_info in reversed(self.global_packets):
                    if packet_info['timestamp'] < start_time:
                        continue  # Skip old packets
                    
                    pkt = packet_info['pkt']
                    if (pkt.haslayer(TCP) and 
                        pkt[TCP].dport == self.sport and 
                        pkt[TCP].flags & 0x02 == 0x02):  # SYN flag
                        print(f"[DEBUG] Found SYN in global buffer from {packet_info['timestamp']:.6f}")
                        self.client_ip = pkt[IP].src
                        self.client_port = pkt[TCP].sport
                        self.ack = pkt[TCP].seq + 1
                        self.seq = random.randrange(0, (2**32) - 1)
                        print(f"[SERVER] Got SYN from {self.client_ip}:{self.client_port}")
                        print(f"[DEBUG] Initial SEQ={self.seq}, ACK={self.ack}")
                        return pkt
        
        return None

    def _send_synack(self):
        ip = IP(dst=self.client_ip)
        synack = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='SA')
        print(f"[SERVER] Sending SYN/ACK (SEQ={self.seq}, ACK={self.ack})")
        send(synack, verbose=0, iface=self.iface)
        print(f"[DEBUG] SYN/ACK sent: {synack.summary()}")

    def _wait_for_ack(self):
        print(f"[SERVER] Waiting for ACK from {self.client_ip}:{self.client_port}...")
        
        start_time = time.time()
        print(f"[DEBUG] {start_time:.6f} Starting to look for handshake ACK packet")
        
        def is_handshake_ack(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt[IP].src == self.client_ip
                and pkt[TCP].sport == self.client_port
                and pkt[TCP].dport == self.sport
                and pkt[TCP].flags & 0x10 == 0x10  # Has ACK flag
                and not pkt.haslayer(Raw)  # But no payload (pure ACK)
                and pkt[TCP].ack == self.seq + 1  # And ACKs our SYN/ACK
            )
        
        # Check if we already have the ACK packet in our global buffer
        with self.global_packets_lock:
            for packet_info in reversed(self.global_packets):  # Check most recent first
                pkt = packet_info['pkt']
                if is_handshake_ack(pkt):
                    print(f"[DEBUG] Found handshake ACK in global buffer from {packet_info['timestamp']:.6f}")
                    print(f"[SERVER] Got ACK, handshake complete.")
                    self.seq += 1
                    print(f"[DEBUG] Updated SEQ={self.seq}, ACK={self.ack}")
                    return True
        
        # If not found in buffer, wait for new packets
        print(f"[DEBUG] Handshake ACK not in buffer, waiting for new packets...")
        timeout = 10
        while (time.time() - start_time) < timeout and self.running:
            time.sleep(0.1)  # Check every 100ms
            
            with self.global_packets_lock:
                for packet_info in reversed(self.global_packets):
                    if packet_info['timestamp'] < start_time:
                        continue  # Skip old packets
                    
                    pkt = packet_info['pkt']
                    if is_handshake_ack(pkt):
                        print(f"[DEBUG] Found handshake ACK in global buffer from {packet_info['timestamp']:.6f}")
                        print(f"[SERVER] Got ACK, handshake complete.")
                        self.seq += 1
                        print(f"[DEBUG] Updated SEQ={self.seq}, ACK={self.ack}")
                        return True
        
        print("[SERVER] ACK not received, aborting.")
        return False

    def _wait_for_get(self):
        print(f"[SERVER] Waiting for HTTP GET from {self.client_ip}:{self.client_port}...")
        
        # Check if we already have the packet in our global capture
        start_time = time.time()
        print(f"[DEBUG] {start_time:.6f} Starting to look for HTTP GET packet")
        
        # First, check if packet is already in our global buffer
        with self.global_packets_lock:
            for packet_info in reversed(self.global_packets):  # Check most recent first
                pkt = packet_info['pkt']
                if (pkt.haslayer(TCP) and pkt.haslayer(Raw) and 
                    pkt[IP].src == self.client_ip and 
                    pkt[TCP].sport == self.client_port and 
                    pkt[TCP].dport == self.sport and 
                    b"GET" in pkt[Raw].load):
                    print(f"[DEBUG] Found HTTP GET in global buffer from {packet_info['timestamp']:.6f}")
                    self.ack = pkt[TCP].seq + len(pkt[Raw].load)
                    print(f"[DEBUG] Updated ACK={self.ack}")
                    return pkt
        
        # If not found in buffer, wait for new packets
        print(f"[DEBUG] HTTP GET not in buffer, waiting for new packets...")
        timeout = 10
        while (time.time() - start_time) < timeout and self.running:
            time.sleep(0.1)  # Check every 100ms
            
            with self.global_packets_lock:
                for packet_info in reversed(self.global_packets):
                    if packet_info['timestamp'] < start_time:
                        continue  # Skip old packets
                    
                    pkt = packet_info['pkt']
                    if (pkt.haslayer(TCP) and pkt.haslayer(Raw) and 
                        pkt[IP].src == self.client_ip and 
                        pkt[TCP].sport == self.client_port and 
                        pkt[TCP].dport == self.sport and 
                        b"GET" in pkt[Raw].load):
                        print(f"[DEBUG] Found HTTP GET in global buffer from {packet_info['timestamp']:.6f}")
                        print(f"[DEBUG] HTTP GET payload: {pkt[Raw].load[:100]!r}")
                        self.ack = pkt[TCP].seq + len(pkt[Raw].load)
                        print(f"[DEBUG] Updated ACK={self.ack}")
                        return pkt
        
        print("[SERVER] No HTTP GET received within timeout.")
        return None

    def _send_ack(self):
        ip = IP(dst=self.client_ip)
        ack_pkt = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='A')
        print(f"[SERVER] Sending ACK for HTTP GET (SEQ={self.seq}, ACK={self.ack})")
        send(ack_pkt, verbose=0, iface=self.iface)
        print(f"[DEBUG] ACK sent: {ack_pkt.summary()}")

    def _wait_for_data_ack(self, expected_ack):
        """Wait for ACK from client for data segment"""
        print(f"[SERVER] Waiting for data ACK (expecting ACK={expected_ack}) from {self.client_ip}:{self.client_port}...")
        
        start_time = time.time()
        print(f"[DEBUG] {start_time:.6f} Starting to look for data ACK packet")
        
        def is_data_ack(pkt):
            return (
                pkt.haslayer(TCP)
                and pkt[IP].src == self.client_ip
                and pkt[TCP].sport == self.client_port
                and pkt[TCP].dport == self.sport
                and pkt[TCP].flags & 0x10 == 0x10  # Has ACK flag
                and pkt[TCP].ack == expected_ack  # ACKs our data
            )
        
        # Check if we already have the ACK packet in our global buffer
        with self.global_packets_lock:
            for packet_info in reversed(self.global_packets):  # Check most recent first
                pkt = packet_info['pkt']
                if is_data_ack(pkt):
                    print(f"[DEBUG] Found data ACK in global buffer from {packet_info['timestamp']:.6f}")
                    print(f"[SERVER] Got data ACK (ACK={pkt[TCP].ack})")
                    return True
        
        # If not found in buffer, wait for new packets
        print(f"[DEBUG] Data ACK not in buffer, waiting for new packets...")
        timeout = 10
        while (time.time() - start_time) < timeout and self.running:
            time.sleep(0.1)  # Check every 100ms
            
            with self.global_packets_lock:
                for packet_info in reversed(self.global_packets):
                    if packet_info['timestamp'] < start_time:
                        continue  # Skip old packets
                    
                    pkt = packet_info['pkt']
                    if is_data_ack(pkt):
                        print(f"[DEBUG] Found data ACK in global buffer from {packet_info['timestamp']:.6f}")
                        print(f"[SERVER] Got data ACK (ACK={pkt[TCP].ack})")
                        return True
        
        print("[SERVER] Data ACK not received within timeout.")
        return False

    def _send_http_response(self):
        ip = IP(dst=self.client_ip)
        headers = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(self.response_data)}\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).encode()
        full_payload = headers + self.response_data
        print(f"[SERVER] Sending HTTP response in 3 segments ({len(full_payload)} bytes total: {len(headers)} headers + {len(self.response_data)} data) to {self.client_ip}:{self.client_port}")
        
        # Split payload into 3 chunks: 500, 500, rest
        chunk_sizes = [500, 500]
        remaining = len(full_payload) - sum(chunk_sizes)
        if remaining > 0:
            chunk_sizes.append(remaining)
        
        print(f"[DEBUG] Chunk sizes: {chunk_sizes}")
        
        current_pos = 0
        for i, chunk_size in enumerate(chunk_sizes, 1):
            if current_pos >= len(full_payload):
                break
                
            # Get the chunk data
            chunk_end = min(current_pos + chunk_size, len(full_payload))
            chunk_data = full_payload[current_pos:chunk_end]
            actual_chunk_size = len(chunk_data)
            
            print(f"[SERVER] Sending segment {i}/{len(chunk_sizes)} ({actual_chunk_size} bytes) SEQ={self.seq}")
            
            # Send the chunk
            psh = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='PA')/chunk_data
            send(psh, verbose=0, iface=self.iface)
            print(f"[DEBUG] Segment {i} sent: {psh.summary()}")
            
            # Update sequence number
            next_seq = self.seq + actual_chunk_size
            print(f"[DEBUG] Segment {i} SEQ={self.seq}, next SEQ={next_seq}, ACK={self.ack}")
            
            # Wait for ACK before sending next segment (except for the last one)
            if i < len(chunk_sizes):
                if not self._wait_for_data_ack(next_seq):
                    print(f"[WARNING] Failed to receive ACK for segment {i}, continuing anyway...")
            
            self.seq = next_seq
            current_pos = chunk_end
        
        print(f"[SERVER] All HTTP response segments sent. Final SEQ={self.seq}")

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
        
        # Use shorter timeouts
        timeout_count = 0
        max_timeouts = 120  # 120 * 0.5 seconds = 60 seconds total
        while self.running and timeout_count < max_timeouts:
            try:
                pkts = sniff(filter=f"tcp and src host {self.client_ip} and src port {self.client_port} and dst port {self.sport}", lfilter=fin_filter, count=1, timeout=0.5, iface=self.iface)
                if pkts:
                    print(f"[DEBUG] FIN packet sniffed: {pkts[0].summary()}")
                    self.ack = pkts[0][TCP].seq + 1
                    print(f"[SERVER] Got FIN, sending FIN/ACK and closing.")
                    ip = IP(dst=self.client_ip)
                    finack = ip/TCP(sport=self.sport, dport=self.client_port, seq=self.seq, ack=self.ack, flags='FA')
                    send(finack, verbose=0, iface=self.iface)
                    print(f"[DEBUG] FIN/ACK sent: {finack.summary()}")
                    print(f"[DEBUG] Final SEQ={self.seq}, ACK={self.ack}")
                    return
                timeout_count += 1
            except KeyboardInterrupt:
                print("[SERVER] KeyboardInterrupt during FIN wait. Exiting.")
                self.running = False
                return
            except Exception as e:
                print(f"[SERVER] Error during FIN wait: {e}")
                return
        
        print("[SERVER] No FIN received, closing.")
        return

    def run(self):
        # Start global packet sniffer
        self._start_global_sniffer()
        
        while self.running:
            self.seq = None
            self.ack = None
            self.client_ip = None
            self.client_port = None
            try:
                print(f"[TIMING] {time.time():.6f} Starting new connection cycle")
                
                syn_pkt = self._wait_for_syn()
                if syn_pkt is None:
                    if not self.running:
                        break
                    continue
                
                if not self.running:
                    break
                
                print(f"[TIMING] {time.time():.6f} Sending SYN/ACK")
                self._send_synack()
                
                if not self._wait_for_ack():
                    continue
                    
                if not self.running:
                    break
                
                print(f"[TIMING] {time.time():.6f} Handshake complete, waiting for HTTP GET")
                get_pkt = self._wait_for_get()
                if not get_pkt:
                    continue
                    
                if not self.running:
                    break
                
                print(f"[TIMING] {time.time():.6f} Got HTTP GET, sending ACK and response")
                self._send_ack()
                self._send_http_response()
                self._wait_for_fin()
                print(f"[SERVER] Session complete. Ready for next connection.\n")
            except KeyboardInterrupt:
                print("[SERVER] Shutting down.")
                self.running = False
                break
            except Exception as e:
                print(f"[SERVER] Error: {e}")
                if not self.running:
                    break
                continue
        
        print("[SERVER] Server stopped.")

def main():
    parser = argparse.ArgumentParser(description='Scapy TCP Server for HTTP Window Testing')
    parser.add_argument('--port', type=int, default=DEF_PORT, help='Server port')
    parser.add_argument('--iface', type=str, default='lo', help='Network interface to sniff/send on (default: lo)')
    args = parser.parse_args()
    print(f"[SCAPY TCP SERVER] Listening on port {args.port} (iface={args.iface})")
    print(f"[SCAPY TCP SERVER] Make sure to block kernel RSTs with iptables!")
    print(f"[SCAPY TCP SERVER] Press Ctrl+C to stop the server.")
    
    try:
        server = ScapyTcpServer(port=args.port, iface=args.iface)
        server.run()
    except KeyboardInterrupt:
        print("\n[MAIN] Received KeyboardInterrupt, exiting gracefully.")
        sys.exit(0)
    except Exception as e:
        print(f"[MAIN] Fatal error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main() 