#!/usr/bin/env python3
"""
TCP Test Client for ESP32 lwIP Window Overrun Testing

This client connects to the Scapy TCP server and can be configured
to simulate various scenarios that might trigger the tcplen > rcv_wnd condition.

Usage:
    python3 test_tcp_client.py --server 127.0.0.1 --port 3333 --mode normal
    python3 test_tcp_client.py --server 192.168.1.100 --port 3333 --mode stress_window
"""

import argparse
import socket
import time
import threading
import struct

class TcpTestClient:
    def __init__(self, server_ip, server_port, mode='normal'):
        self.server_ip = server_ip
        self.server_port = server_port
        self.mode = mode
        self.sock = None
        
    def connect(self):
        """Establish connection to the server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
            # Configure socket options based on mode
            if self.mode == 'small_buffer':
                # Set small receive buffer to trigger window issues
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
                print("Set small receive buffer: 1024 bytes")
                
            elif self.mode == 'tiny_buffer':
                # Set very small receive buffer
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 512)
                print("Set tiny receive buffer: 512 bytes")
                
            elif self.mode == 'no_delay':
                # Disable Nagle's algorithm
                self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print("Disabled Nagle's algorithm")
            
            print(f"Connecting to {self.server_ip}:{self.server_port}")
            self.sock.connect((self.server_ip, self.server_port))
            print("Connected successfully!")
            
            return True
            
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def send_request(self):
        """Send HTTP-like GET request"""
        request = "GET /test HTTP/1.1\r\nHost: test\r\nConnection: close\r\n\r\n"
        try:
            print(f"Sending request: {len(request)} bytes")
            self.sock.send(request.encode())
            return True
        except Exception as e:
            print(f"Send failed: {e}")
            return False
    
    def receive_data_normal(self):
        """Normal receive - read all data"""
        total_data = b""
        start_time = time.time()
        
        try:
            while True:
                self.sock.settimeout(5.0)
                data = self.sock.recv(4096)
                if not data:
                    break
                    
                total_data += data
                print(f"Received chunk: {len(data)} bytes (total: {len(total_data)})")
                
        except socket.timeout:
            print("Receive timeout")
        except Exception as e:
            print(f"Receive error: {e}")
            
        elapsed = time.time() - start_time
        print(f"Total received: {len(total_data)} bytes in {elapsed:.2f}s")
        return total_data
    
    def receive_data_slow(self):
        """Slow receive - read small chunks with delays"""
        total_data = b""
        chunk_size = 100  # Very small chunks
        
        try:
            while True:
                self.sock.settimeout(2.0)
                data = self.sock.recv(chunk_size)
                if not data:
                    break
                    
                total_data += data
                print(f"Slow receive: {len(data)} bytes (total: {len(total_data)})")
                
                # Delay to simulate slow processing
                time.sleep(0.1)
                
        except socket.timeout:
            print("Slow receive timeout")
        except Exception as e:
            print(f"Slow receive error: {e}")
            
        print(f"Slow receive total: {len(total_data)} bytes")
        return total_data
    
    def receive_data_stall(self):
        """Stall receive - read some data then stop to cause window issues"""
        total_data = b""
        
        try:
            # Read first chunk normally
            data = self.sock.recv(500)
            if data:
                total_data += data
                print(f"Initial receive: {len(data)} bytes")
                
                # Now stall - don't read more data for a while
                print("Stalling receive for 2 seconds...")
                time.sleep(2.0)
                
                # Try to read more
                while True:
                    self.sock.settimeout(1.0)
                    data = self.sock.recv(4096)
                    if not data:
                        break
                    total_data += data
                    print(f"After stall: {len(data)} bytes (total: {len(total_data)})")
                
        except socket.timeout:
            print("Stall receive timeout")
        except Exception as e:
            print(f"Stall receive error: {e}")
            
        print(f"Stall receive total: {len(total_data)} bytes")
        return total_data
    
    def run_test(self):
        """Run the complete test"""
        print(f"=== TCP Test Client - Mode: {self.mode} ===")
        
        if not self.connect():
            return False
            
        try:
            # Send request
            if not self.send_request():
                return False
            
            # Receive response based on mode
            if self.mode in ['slow_receive', 'small_buffer', 'tiny_buffer']:
                data = self.receive_data_slow()
            elif self.mode == 'stall_receive':
                data = self.receive_data_stall()
            else:
                data = self.receive_data_normal()
            
            # Print some stats about received data
            if data:
                print(f"\n=== Received Data Analysis ===")
                print(f"Total bytes: {len(data)}")
                if len(data) > 100:
                    print(f"First 100 bytes: {data[:100]}")
                    print(f"Last 100 bytes: {data[-100:]}")
                else:
                    print(f"All data: {data}")
                
                # Check for expected test pattern
                if b"SCAPY_TCP_TEST_" in data:
                    print("✓ Found expected test data pattern")
                else:
                    print("✗ Expected test data pattern not found")
            
            return True
            
        finally:
            self.close()
    
    def close(self):
        """Close the connection"""
        if self.sock:
            try:
                self.sock.close()
                print("Connection closed")
            except:
                pass

def main():
    parser = argparse.ArgumentParser(description='TCP Test Client for lwIP Window Testing')
    parser.add_argument('--server', default='127.0.0.1', help='Server IP address')
    parser.add_argument('--port', type=int, default=3333, help='Server port')
    parser.add_argument('--mode', 
                       choices=['normal', 'slow_receive', 'stall_receive', 'small_buffer', 'tiny_buffer', 'no_delay'],
                       default='normal',
                       help='Test mode')
    parser.add_argument('--repeat', type=int, default=1, help='Number of test repetitions')
    
    args = parser.parse_args()
    
    print("TCP Test Client Modes:")
    print("  normal: Standard TCP client behavior")
    print("  slow_receive: Read data in small chunks with delays")
    print("  stall_receive: Read some data, then stall, then continue")
    print("  small_buffer: Use small socket receive buffer (1024 bytes)")
    print("  tiny_buffer: Use tiny socket receive buffer (512 bytes)")
    print("  no_delay: Disable Nagle's algorithm")
    print()
    
    success_count = 0
    
    for i in range(args.repeat):
        if args.repeat > 1:
            print(f"\n=== Test Run {i+1}/{args.repeat} ===")
        
        client = TcpTestClient(args.server, args.port, args.mode)
        
        try:
            if client.run_test():
                success_count += 1
            else:
                print("Test failed!")
                
        except KeyboardInterrupt:
            print("\nTest interrupted by user")
            break
        except Exception as e:
            print(f"Test error: {e}")
        
        # Short delay between repetitions
        if i < args.repeat - 1:
            time.sleep(1)
    
    if args.repeat > 1:
        print(f"\n=== Test Summary ===")
        print(f"Successful runs: {success_count}/{args.repeat}")

if __name__ == '__main__':
    main() 