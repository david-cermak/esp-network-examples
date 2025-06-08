#!/bin/bash

# Radamsa fuzzing script for IoT Network Message Parser
# Usage: ./run_fuzzer.sh [-n number_of_runs] [-t timeout_seconds]

set -e

# Default values
NUM_RUNS=1000
TIMEOUT=5
TARGET_BINARY="../sample/build/iot_parser"
SAMPLE_FILE="sample_inputs.txt"
CRASHES_DIR="crashes"
TIMEOUTS_DIR="timeouts"
ERRORS_DIR="errors"
LOG_FILE="fuzzing.log"

# Parse command line arguments
while getopts "n:t:h" opt; do
    case $opt in
        n) NUM_RUNS="$OPTARG" ;;
        t) TIMEOUT="$OPTARG" ;;
        h) echo "Usage: $0 [-n runs] [-t timeout] [-h help]"
           echo "  -n: Number of fuzzing runs (default: 1000)"
           echo "  -t: Timeout per run in seconds (default: 5)"
           echo "  -h: Show this help"
           exit 0 ;;
        *) echo "Invalid option. Use -h for help."; exit 1 ;;
    esac
done

# Check if target binary exists
if [ ! -f "$TARGET_BINARY" ]; then
    echo "Error: Target binary not found at $TARGET_BINARY"
    echo "Please build the IoT parser first:"
    echo "  cd ../sample && mkdir build && cd build && cmake .. && make"
    exit 1
fi

# Check if radamsa is available
if ! command -v radamsa &> /dev/null; then
    echo "Error: radamsa not found in PATH"
    echo "Please install radamsa first"
    exit 1
fi

# Create directories
mkdir -p "$CRASHES_DIR" "$TIMEOUTS_DIR" "$ERRORS_DIR"

# Create sample input file if it doesn't exist
if [ ! -f "$SAMPLE_FILE" ]; then
    echo "Creating sample input file..."
    cat > "$SAMPLE_FILE" << 'EOF'
mqtt/home/temperature {"device_id":"sensor01","temperature":23.5,"status":"active"}
mqtt/device/led {"action":"toggle","brightness":80,"color":"red"}
mqtt/sensors/humidity {"device_id":"esp32_01","humidity":65,"location":"kitchen"}
GET /api/sensors {"action":"read","sensor":"temperature"}
POST /device/control {"action":"set","value":"on","device":"fan"}
GET /status {"device_id":"thermostat","action":"get_state"}
POST /api/data {"temperature":22.5,"humidity":60,"timestamp":1234567890}
mqtt/alarm/motion {"sensor":"pir_01","status":"detected","room":"living_room"}
EOF
fi

echo "Starting radamsa fuzzing..."
echo "Target: $TARGET_BINARY"
echo "Runs: $NUM_RUNS"
echo "Timeout: ${TIMEOUT}s per run"
echo "Sample file: $SAMPLE_FILE"
echo "Crashes will be saved to: $CRASHES_DIR/"
echo "Timeouts will be saved to: $TIMEOUTS_DIR/"
echo "Other errors will be saved to: $ERRORS_DIR/"
echo "Log file: $LOG_FILE"
echo "=========================================="

# Initialize counters
crashes=0
timeouts=0
errors=0
runs=0

# Create temporary file for fuzzed input
TEMP_INPUT=$(mktemp)
trap "rm -f $TEMP_INPUT" EXIT

# Start fuzzing
for ((i=1; i<=NUM_RUNS; i++)); do
    runs=$((runs + 1))
    
    # Generate fuzzed input to temporary file
    radamsa "$SAMPLE_FILE" > "$TEMP_INPUT"
    
    # Run the target with timeout
    exit_code=0
    if timeout "$TIMEOUT" bash -c "$TARGET_BINARY < $TEMP_INPUT" &>/dev/null; then
        # Normal execution
        echo -n "."
    else
        exit_code=$?
        if [ $exit_code -eq 124 ]; then
            # Timeout
            timeouts=$((timeouts + 1))
            timeout_file="$TIMEOUTS_DIR/timeout_$(date +%s)_$i.txt"
            cp "$TEMP_INPUT" "$timeout_file"
            echo -n "T"
            echo "$(date): Timeout #$timeouts - Run $i - Exit code: $exit_code" >> "$LOG_FILE"
            echo "Input saved to: $timeout_file" >> "$LOG_FILE"
        elif [ $exit_code -gt 127 ]; then
            # Crash (signal)
            crashes=$((crashes + 1))
            crash_file="$CRASHES_DIR/crash_$(date +%s)_$i.txt"
            cp "$TEMP_INPUT" "$crash_file"
            echo -n "C"
            echo "$(date): Crash #$crashes - Run $i - Exit code: $exit_code" >> "$LOG_FILE"
            echo "Input saved to: $crash_file" >> "$LOG_FILE"
        else
            # Other error
            errors=$((errors + 1))
            error_file="$ERRORS_DIR/error_$(date +%s)_$i.txt"
            cp "$TEMP_INPUT" "$error_file"
            echo -n "E"
            echo "$(date): Error #$errors - Run $i - Exit code: $exit_code" >> "$LOG_FILE"
            echo "Input saved to: $error_file" >> "$LOG_FILE"
        fi
    fi
    
    # Progress report every 100 runs
    if [ $((i % 100)) -eq 0 ]; then
        echo ""
        echo "Progress: $i/$NUM_RUNS runs - Crashes: $crashes - Timeouts: $timeouts - Errors: $errors"
    fi
done

echo ""
echo "=========================================="
echo "Fuzzing completed!"
echo "Total runs: $runs"
echo "Crashes found: $crashes"
echo "Timeouts: $timeouts"
echo "Other errors: $errors"
echo "Success rate: $(echo "scale=2; ($runs - $crashes - $timeouts - $errors) * 100 / $runs" | bc -l)%"

if [ $crashes -gt 0 ]; then
    echo ""
    echo "Crash files saved in: $CRASHES_DIR/"
fi

if [ $timeouts -gt 0 ]; then
    echo "Timeout files saved in: $TIMEOUTS_DIR/"
fi

if [ $errors -gt 0 ]; then
    echo "Error files saved in: $ERRORS_DIR/"
fi

if [ $crashes -gt 0 ] || [ $timeouts -gt 0 ] || [ $errors -gt 0 ]; then
    echo ""
    echo "Check the log file for details: $LOG_FILE"
fi
