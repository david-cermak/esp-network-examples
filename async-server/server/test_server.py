#!/usr/bin/env python3
"""
Test script for ESP32 async server implementations.
Tests both pthread and coroutine versions by creating multiple concurrent clients.
"""

import socket
import threading
import time
import sys
import argparse
from concurrent.futures import ThreadPoolExecutor
import statistics

class ServerTester:
    def __init__(self, server_ip, port=8080):
        self.server_ip = server_ip
        self.port = port
        self.results = []
        self.lock = threading.Lock()
    
    def test_single_client(self, client_id):
        """Test a single client connection and measure performance."""
        start_time = time.time()
        total_bytes = 0
        chunks_received = 0
        
        try:
            # Create socket and connect
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(30)  # 30 second timeout
            sock.connect((self.server_ip, self.port))
            
            # Receive HTTP response
            response = b""
            while True:
                try:
                    chunk = sock.recv(1024)
                    if not chunk:
                        break
                    response += chunk
                    chunks_received += 1
                    total_bytes += len(chunk)
                except socket.timeout:
                    break
            
            sock.close()
            
            # Calculate metrics
            end_time = time.time()
            duration = end_time - start_time
            throughput = total_bytes / duration if duration > 0 else 0
            
            # Check if we received valid HTTP response
            is_valid = response.startswith(b"HTTP/1.1 200") and b"Content-Length:" in response
            
            result = {
                'client_id': client_id,
                'duration': duration,
                'total_bytes': total_bytes,
                'chunks_received': chunks_received,
                'throughput_bps': throughput,
                'success': is_valid,
                'response_preview': response[:100].decode('utf-8', errors='ignore')
            }
            
            with self.lock:
                self.results.append(result)
            
            print(f"Client {client_id:3d}: {duration:6.2f}s, {total_bytes:6d} bytes, "
                  f"{throughput:8.1f} B/s, {'✓' if is_valid else '✗'}")
            
        except Exception as e:
            result = {
                'client_id': client_id,
                'duration': 0,
                'total_bytes': 0,
                'chunks_received': 0,
                'throughput_bps': 0,
                'success': False,
                'error': str(e)
            }
            
            with self.lock:
                self.results.append(result)
            
            print(f"Client {client_id:3d}: ERROR - {e}")
    
    def run_concurrent_test(self, num_clients, max_workers=None):
        """Run concurrent test with specified number of clients."""
        print(f"\n=== Starting concurrent test with {num_clients} clients ===")
        print(f"Server: {self.server_ip}:{self.port}")
        print(f"Max concurrent connections: {max_workers or num_clients}")
        print("-" * 70)
        
        start_time = time.time()
        
        # Use ThreadPoolExecutor for controlled concurrency
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            # Submit all client tasks
            futures = [executor.submit(self.test_single_client, i) for i in range(num_clients)]
            
            # Wait for all to complete
            for future in futures:
                future.result()
        
        end_time = time.time()
        total_duration = end_time - start_time
        
        # Print summary
        self.print_summary(total_duration)
    
    def print_summary(self, total_duration):
        """Print test results summary."""
        print("\n" + "=" * 70)
        print("TEST RESULTS SUMMARY")
        print("=" * 70)
        
        successful = [r for r in self.results if r['success']]
        failed = [r for r in self.results if not r['success']]
        
        print(f"Total clients: {len(self.results)}")
        print(f"Successful: {len(successful)}")
        print(f"Failed: {len(failed)}")
        print(f"Total test duration: {total_duration:.2f} seconds")
        
        if successful:
            durations = [r['duration'] for r in successful]
            throughputs = [r['throughput_bps'] for r in successful]
            total_bytes = sum(r['total_bytes'] for r in successful)
            
            print(f"\nSuccessful connections:")
            print(f"  Total data received: {total_bytes:,} bytes")
            print(f"  Average duration: {statistics.mean(durations):.2f}s")
            print(f"  Min duration: {min(durations):.2f}s")
            print(f"  Max duration: {max(durations):.2f}s")
            print(f"  Duration std dev: {statistics.stdev(durations):.2f}s")
            print(f"  Average throughput: {statistics.mean(throughputs):.1f} B/s")
            print(f"  Overall throughput: {total_bytes/total_duration:.1f} B/s")
        
        if failed:
            print(f"\nFailed connections:")
            for r in failed:
                error_msg = r.get('error', 'Unknown error')
                print(f"  Client {r['client_id']}: {error_msg}")
        
        # Performance analysis
        if len(successful) > 1:
            print(f"\nPerformance Analysis:")
            print(f"  Concurrent clients handled: {len(successful)}")
            print(f"  Average time per client: {total_duration/len(successful):.2f}s")
            
            # Check for timing patterns (should see ~1s intervals due to server delays)
            if len(successful) >= 3:
                sorted_durations = sorted(durations)
                print(f"  Timing pattern analysis:")
                for i in range(1, min(4, len(sorted_durations))):
                    diff = sorted_durations[i] - sorted_durations[i-1]
                    print(f"    Client {i} vs {i-1}: {diff:.2f}s difference")

def main():
    parser = argparse.ArgumentParser(description='Test ESP32 async server implementations')
    parser.add_argument('server_ip', help='IP address of the ESP32 server')
    parser.add_argument('num_clients', type=int, help='Number of concurrent clients to test')
    parser.add_argument('--port', type=int, default=8080, help='Server port (default: 8080)')
    parser.add_argument('--max-workers', type=int, help='Maximum concurrent connections (default: same as num_clients)')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.num_clients < 1:
        print("Error: Number of clients must be at least 1")
        sys.exit(1)
    
    if args.max_workers and args.max_workers < 1:
        print("Error: Max workers must be at least 1")
        sys.exit(1)
    
    # Create tester and run test
    tester = ServerTester(args.server_ip, args.port)
    
    try:
        tester.run_concurrent_test(args.num_clients, args.max_workers)
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\nTest failed with error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
