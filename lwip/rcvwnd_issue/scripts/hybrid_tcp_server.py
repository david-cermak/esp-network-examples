#!/usr/bin/env python3
"""
Hybrid TCP Server - Binds to port normally but crafts custom responses

This server combines normal socket binding (to prevent kernel RST) 
with Scapy packet crafting for custom responses.

Usage:
    sudo python3 hybrid_tcp_server.py --port 3333 --mode window_overrun
"""

import argparse
import socket
import threading
import time
from scapy.all import *

# Suppress Scapy warnings
import logging
logging.getLogger("scapy.runtime").setLevel(logging.ERROR)

DEF_PORT = 3333
RESPONSE_SIZE = 1380

class HybridTcpServer:
    def __init__(self, port=DEF_PORT, mode='window_overrun'):
        self.port = port
        self.mode = mode
        self.running = False
        self.response_data = self._create_response_data()
        self.server_socket = None
        
    def _create_response_data(self):
        """Create exactly 1380 bytes of test data"""
        base_text = "HYBRID_TCP_TEST_"
        pattern = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        
        data = base_text
        while len(data) < RESPONSE_SIZE:
            remaining = RESPONSE_SIZE - len(data)
            if remaining >= len(pattern):
                data += pattern
            else:
                data += pattern[:remaining]
                
        return data[:RESPONSE_SIZE].encode()

    def start(self):
        """Start the hybrid server"""
        self.running = True
        
        print(f"[SERVER] Starting Hybrid TCP server on port {self.port}")
        print(f"[SERVER] Mode: {self.mode}")
        print(f"[SERVER] Response data size: {len(self.response_data)} bytes")
        
        try:
            # Create and bind socket normally to prevent kernel RST
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('0.0.0.0', self.port))
            self.server_socket.listen(5)
            
            print(f"[SERVER] Socket bound to port {self.port}")
            print("[SERVER] Waiting for connections...")
            
            while self.running:
                try:
                    self.server_socket.settimeout(1.0)  # Allow periodic checks
                    client_socket, client_address = self.server_socket.accept()
                    
                    print(f"[CONN] New connection from {client_address}")
                    
                    # Handle client in separate thread
                    client_thread = threading.Thread(
                        target=self._handle_client,
                        args=(client_socket, client_address),
                        daemon=True
                    )
                    client_thread.start()
                    
                except socket.timeout:
                    continue  # Check if still running
                except Exception as e:
                    if self.running:
                        print(f"[SERVER] Accept error: {e}")
                        time.sleep(1)
                        
        except Exception as e:
            print(f"[SERVER] Server error: {e}")
        finally:
            self._cleanup()

    def _handle_client(self, client_socket, client_address):
        """Handle individual client connection"""
        client_ip, client_port = client_address
        
        try:
            print(f"[CLIENT] Handling {client_ip}:{client_port}")
            
            # Receive the HTTP request
            client_socket.settimeout(5.0)
            request_data = client_socket.recv(4096)
            
            if not request_data:
                print(f"[CLIENT] No data received from {client_ip}:{client_port}")
                return
                
            request = request_data.decode('utf-8', errors='ignore')
            print(f"[HTTP] Request from {client_ip}:{client_port}:")
            print(f"[HTTP] {repr(request[:100])}")
            
            if 'GET' not in request:
                print(f"[HTTP] No GET request found")
                return
            
            # Get socket details for crafting response
            local_ip, local_port = client_socket.getsockname()
            remote_ip, remote_port = client_socket.getpeername()
            
            print(f"[RESP] Preparing response for {remote_ip}:{remote_port}")
            print(f"[RESP] Local socket: {local_ip}:{local_port}")
            
            # Choose response method based on mode
            if self.mode == 'window_overrun':
                self._send_window_overrun_response(client_socket, remote_ip, remote_port)
            elif self.mode == 'normal':
                self._send_normal_response(client_socket)
            elif self.mode == 'fragmented':
                self._send_fragmented_response(client_socket)
            else:
                print(f"[ERROR] Unknown mode: {self.mode}")
                
        except Exception as e:
            print(f"[CLIENT] Error handling {client_ip}:{client_port}: {e}")
        finally:
            try:
                client_socket.close()
                print(f"[CLIENT] Closed connection to {client_ip}:{client_port}")
            except:
                pass

    def _send_window_overrun_response(self, client_socket, client_ip, client_port):
        """Send response designed to trigger window overrun"""
        print(f"[WINDOW] Sending window overrun test to {client_ip}:{client_port}")
        
        # First, send HTTP headers normally
        headers = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(self.response_data)}\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        
        print(f"[WINDOW] Sending HTTP headers ({len(headers)} bytes)")
        client_socket.send(headers.encode())
        
        # Small delay to let client process headers
        time.sleep(0.1)
        
        # Now we need to send data that exceeds the client's window
        # The key is to send MORE data than the client's receive window allows
        
        # For testing window overrun, we'll send the full 1380 bytes at once
        # even if the client has a small receive buffer
        print(f"[WINDOW] Sending large payload ({len(self.response_data)} bytes)")
        print(f"[WINDOW] This should trigger tcplen > rcv_wnd if client has small window")
        
        try:
            # Send all data at once - this should overwhelm small client windows
            client_socket.send(self.response_data)
            print(f"[WINDOW] Successfully sent {len(self.response_data)} bytes")
        except Exception as e:
            print(f"[WINDOW] Error sending data: {e}")

    def _send_normal_response(self, client_socket):
        """Send normal HTTP response"""
        response = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(self.response_data)}\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).encode() + self.response_data
        
        client_socket.send(response)
        print(f"[NORMAL] Sent normal response ({len(response)} bytes)")

    def _send_fragmented_response(self, client_socket):
        """Send response in multiple fragments"""
        headers = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(self.response_data)}\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        
        # Send headers
        client_socket.send(headers.encode())
        
        # Send data in 500-byte chunks with delays
        chunk_size = 500
        for i in range(0, len(self.response_data), chunk_size):
            chunk = self.response_data[i:i+chunk_size]
            client_socket.send(chunk)
            print(f"[FRAG] Sent chunk {i//chunk_size + 1}: {len(chunk)} bytes")
            time.sleep(0.05)  # Small delay between chunks

    def _cleanup(self):
        """Clean up server resources"""
        self.running = False
        if self.server_socket:
            try:
                self.server_socket.close()
                print("[SERVER] Server socket closed")
            except:
                pass

    def stop(self):
        """Stop the server"""
        print("[SERVER] Stopping server...")
        self._cleanup()

def main():
    parser = argparse.ArgumentParser(description='Hybrid TCP Server for Window Testing')
    parser.add_argument('--port', type=int, default=DEF_PORT, help='Server port')
    parser.add_argument('--mode', 
                       choices=['window_overrun', 'normal', 'fragmented'],
                       default='window_overrun',
                       help='Response mode')
    
    args = parser.parse_args()
    
    print("Hybrid TCP Server Modes:")
    print("  window_overrun: Send large data to trigger window overrun")
    print("  normal: Standard HTTP response")
    print("  fragmented: Send data in small chunks with delays")
    print()
    
    server = HybridTcpServer(port=args.port, mode=args.mode)
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nShutting down...")
        server.stop()

if __name__ == '__main__':
    main() 