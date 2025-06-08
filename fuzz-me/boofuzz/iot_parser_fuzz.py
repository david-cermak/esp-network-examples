#!/usr/bin/env python3
"""
Boofuzz session for IoT Network Message Parser

This script demonstrates protocol-aware fuzzing using boofuzz to test
the IoT parser with structured MQTT and HTTP message fuzzing.

Usage:
    python3 iot_parser_fuzz.py [--target-path PATH] [--index START_INDEX]
"""

import argparse
import os
import sys

try:
    from boofuzz import (
        Session, Target, Request, Static, String, Delim, FuzzLoggerText,
    )
    from boofuzz.connections import BaseSocketConnection
    from boofuzz.monitors import BaseMonitor
    import subprocess
    import socket
except ImportError:
    print("Error: boofuzz not installed. Install with: pip install boofuzz")
    sys.exit(1)

# Create logger for proper boofuzz logging
mylogger = FuzzLoggerText()

class ProcessTargetMonitor(BaseMonitor):
    """
    Monitor that tracks process exit codes and detects crashes
    """
    def __init__(self, connection_ref):
        super().__init__()
        self.connection_ref = connection_ref
    
    def alive(self):
        """Called when target is added to session - monitor setup check"""
        mylogger.log_info("Monitor alive() called - monitor is ready")
        return True  # Monitor itself is always alive
    
    def pre_send(self, target=None, fuzz_data_logger=None, session=None, **kwargs):
        """Called before each test case"""
        mylogger.log_info("pre_send() called - preparing for test")
    
    def post_send(self, target=None, fuzz_data_logger=None, session=None, **kwargs):
        """Check the result after sending data - THIS is where crash detection happens"""
        mylogger.log_info("post_send() called - checking for crashes")
        
        # Get the exit code from our connection object
        if hasattr(self.connection_ref, 'last_exit_code'):
            last_exit_code = self.connection_ref.last_exit_code
            mylogger.log_error(f"post_send(): got exit code {last_exit_code}")
            
            # Detect crashes based on exit code
            if last_exit_code > 127:
                # Process killed by signal (SIGSEGV, SIGABRT, etc.) - THIS IS A CRASH
                mylogger.log_error(f"CRASH DETECTED: Process killed by signal (exit code {last_exit_code})")
                return False  # Return False = CRASH DETECTED
            elif last_exit_code != 0:
                # Non-zero exit - interesting for logging but NOT a crash (just invalid input)
                mylogger.log_error(f"CRASH DETECTED: Process exited with code {last_exit_code}")
                return False  # Return False = CRASH DETECTED
                return True  # Return True = TARGET STILL ALIVE
            else:
                # Exit code 0 - success
                mylogger.log_info("Process exited successfully (code 0)")
                return True  # Return True = TARGET STILL ALIVE
        
        mylogger.log_info("post_send(): no exit code available, assuming target alive")
        return True  # Default to alive if no exit code
    
    def restart_target(self, target=None, **kwargs):
        """Called when target needs to be restarted after crash"""
        mylogger.log_info("restart_target() called - target restarted")
        return True

class ProcessTargetConnection:
    """
    Simplified connection class - just delivers data to process
    """
    def __init__(self, target_binary):
        self.target_binary = target_binary
        self._process = None
        self.last_exit_code = 0
        self.info = f"Process({target_binary})"
    
    def open(self):
        """Start the target process"""
        return True
    
    def close(self):
        """Clean up any resources"""
        if self._process and self._process.poll() is None:
            self._process.terminate()
    
    def send(self, data):
        """Send data to the target process via stdin"""
        try:
            # Run the process with the fuzzed input
            self._process = subprocess.Popen(
                [self.target_binary],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            
            stdout, stderr = self._process.communicate(input=data, timeout=5)
            self.last_exit_code = self._process.returncode
            mylogger.log_info(f"Process exit code: {self.last_exit_code}")
            # Always return bytes sent - the monitor will check for crashes
            return len(data)
            
        except subprocess.TimeoutExpired:
            if self._process:
                self._process.kill()
            # Mark as timeout crash
            self.last_exit_code = 999  # Special code for timeout
            raise Exception("Process timeout - potential infinite loop or hang")
        except OSError as e:
            # Binary doesn't exist or can't be executed - this is a setup error
            raise Exception(f"Failed to execute target binary: {e}")
    
    def recv(self, max_bytes=1024):
        """Receive data (not needed for our use case)"""
        return b""

def create_mqtt_message():
    """
    Create MQTT message structure: mqtt/topic/path {"key":"value"}
    """
    return Request("mqtt-message", children=(
        Static("protocol", "mqtt"),
        String("topic", "/home/temperature", max_len=200),  # Fuzzable topic
        Delim("space", " "),
        String("json_payload", '{"device_id":"sensor01","temperature":23.5,"status":"active"}', max_len=1000),  # Fuzzable JSON
    ))

def create_mqtt_simple():
    """
    Create simplified MQTT message for broader fuzzing
    """
    return Request("mqtt-simple", children=(
        Static("protocol", "mqtt"),
        String("topic", "/device/led", max_len=500),  # Long topic for overflow testing
        Delim("space", " "),
        String("payload", "{\"action\":\"toggle\"}", max_len=1000),  # Large payload for overflow
    ))

def create_http_get():
    """
    Create HTTP GET message structure: GET /path {"key":"value"}
    """
    return Request("http-get", children=(
        Static("method", "GET"),
        Delim("space1", " "),
        String("path", "/api/sensors", max_len=200),  # Fuzzable path
        Delim("space2", " "),
        String("json_payload", '{"action":"read","sensor":"temperature"}', max_len=800),  # Fuzzable JSON
    ))

def create_http_post():
    """
    Create HTTP POST message structure: POST /path {"key":"value"}
    """
    return Request("http-post", children=(
        Static("method", "POST"),
        Delim("space1", " "),
        String("path", "/device/control", max_len=200),  # Fuzzable path
        Delim("space2", " "),
        String("json_payload", '{"action":"set","value":"on","device":"fan"}', max_len=800),  # Fuzzable JSON
    ))

def create_malformed_messages():
    """
    Create intentionally malformed messages to trigger edge cases
    """
    # Malformed JSON
    malformed_json = Request("malformed-json", children=(
        Static("protocol", "mqtt"),
        String("topic", "/test", max_len=100),
        Delim("space", " "),
        String("bad_json", "{\"unclosed\":\"value", max_len=500),  # Missing closing brace
    ))
    
    # Very long topic
    long_topic = Request("long-topic", children=(
        Static("protocol", "mqtt"),
        String("long_topic", "/" + "A" * 100, max_len=1000),  # Very long topic
        Delim("space", " "),
        String("payload", "{\"test\":\"value\"}", max_len=100),
    ))
    
    # Empty components
    empty_message = Request("empty-parts", children=(
        Static("protocol", "mqtt"),
        String("empty_topic", "", max_len=100),  # Empty topic
        Delim("space", " "),
        String("empty_payload", "", max_len=100),  # Empty payload
    ))
    
    return [malformed_json, long_topic, empty_message]

def test_message_generation():
    """
    Test that all message types generate valid outputs
    """
    mylogger.log_info("Testing message generation...")
    
    messages = [
        ("MQTT Message", create_mqtt_message()),
        ("MQTT Simple", create_mqtt_simple()),
        ("HTTP GET", create_http_get()),
        ("HTTP POST", create_http_post()),
    ]
    
    for name, msg in messages:
        try:
            rendered = msg.render()
            mylogger.log_info(f"✓ {name}: {rendered.decode('utf-8', errors='replace')[:100]}...")
        except Exception as e:
            mylogger.log_error(f"✗ {name}: Error - {e}")
    
    mylogger.log_info("Message generation test completed")

def main():
    parser = argparse.ArgumentParser(description="Boofuzz IoT Parser Fuzzer")
    parser.add_argument(
        "--target-path", 
        default="../sample/build/iot_parser",
        help="Path to the iot_parser binary"
    )
    parser.add_argument(
        "--index",
        type=int,
        default=1,
        help="Test case index to start from (for resuming)"
    )
    parser.add_argument(
        "--web-port",
        type=int,
        default=26000,
        help="Port for boofuzz web interface"
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Test message generation without fuzzing"
    )
    
    args = parser.parse_args()
    
    # Handle test mode
    if args.test:
        test_message_generation()
        return
    
    # Check if target exists
    if not os.path.exists(args.target_path):
        print(f"Error: Target binary not found at {args.target_path}")
        print("Please build the IoT parser first:")
        print("  cd ../sample && mkdir build && cd build")
        print("  cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON .. && make")
        sys.exit(1)
    
    mylogger.log_info("=" * 60)
    mylogger.log_info("Boofuzz IoT Network Message Parser Fuzzer")
    mylogger.log_info("=" * 60)
    mylogger.log_info(f"Target: {args.target_path}")
    mylogger.log_info(f"Web interface: http://localhost:{args.web_port}")
    mylogger.log_info(f"Starting from index: {args.index}")
    mylogger.log_info("=" * 60)
    
    # Create session with our custom process connection and monitor
    connection = ProcessTargetConnection(args.target_path)
    monitor = ProcessTargetMonitor(connection)
    session = Session(
        target=Target(
            connection=connection,
            monitors=[monitor],
        ),
        index_start=args.index,
        web_port=args.web_port,
        crash_threshold_element=10,  # Stop after 10 crashes per element
    )
    
    # Define protocol messages
    mqtt_msg = create_mqtt_message()
    mqtt_simple = create_mqtt_simple()
    http_get = create_http_get()
    http_post = create_http_post()
    malformed_msgs = create_malformed_messages()
    
    # Connect messages to session (no dependencies between them)
    session.connect(mqtt_msg)
    session.connect(mqtt_simple)
    session.connect(http_get)
    session.connect(http_post)
    
    # Add malformed messages
    for msg in malformed_msgs:
        session.connect(msg)
    
    mylogger.log_info("Message definitions:")
    mylogger.log_info("- mqtt-message: Structured MQTT with JSON fields")
    mylogger.log_info("- mqtt-simple: Simple MQTT for overflow testing")
    mylogger.log_info("- http-get: HTTP GET request structure")
    mylogger.log_info("- http-post: HTTP POST request structure")
    mylogger.log_info("- malformed-json: Intentionally broken JSON")
    mylogger.log_info("- long-topic: Very long topic names")
    mylogger.log_info("- empty-parts: Empty fields testing")
    mylogger.log_info("Starting fuzzing session...")
    mylogger.log_info("Use Ctrl+C to stop fuzzing")
    
    # Start fuzzing
    try:
        session.fuzz()
    except KeyboardInterrupt:
        mylogger.log_info("Fuzzing stopped by user")
    except Exception as e:
        mylogger.log_error(f"Fuzzing stopped due to error: {e}")
    
    mylogger.log_info("Fuzzing session completed!")
    mylogger.log_info("Check boofuzz-results/ directory for detailed logs")
    mylogger.log_info(f"Web interface available at: http://localhost:{args.web_port}")

if __name__ == "__main__":
    main() 