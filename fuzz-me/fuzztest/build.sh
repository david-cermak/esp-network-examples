#!/bin/bash

set -e

echo "=== FuzzTest Setup and Build Script ==="

# Clone FuzzTest if it doesn't exist
if [ ! -d "fuzztest" ]; then
    echo "Cloning FuzzTest..."
    git clone https://github.com/google/fuzztest.git
else
    echo "FuzzTest already exists, skipping clone..."
fi

# Create build directory
echo "Setting up build directory..."
mkdir -p build

# ======================================
# Build in Unit Test Mode (Default)
# ======================================
echo ""
echo "=== Building in Unit Test Mode ==="
cd build
CC=clang CXX=clang++ cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  ..

cmake --build .

echo ""
echo "=== Running Unit Tests ==="
echo "This will run both unit tests and fuzz tests for a short time..."
./iot_parser_fuzztest

cd ..

# ======================================
# Build in Fuzzing Mode
# ======================================
echo ""
echo "=== Building in Fuzzing Mode ==="
mkdir -p build_fuzz
cd build_fuzz

CC=clang CXX=clang++ cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFUZZTEST_FUZZING_MODE=on \
  ..

cmake --build .

echo ""
echo "=== Fuzzing Mode Ready ==="
echo "To run individual fuzz tests, use:"
echo "  ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzMqttTopicParsing"
echo "  ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzJsonExtraction"
echo "  ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzHttpRequestParsing"
echo ""
echo "To run for a specific duration (e.g., 30 seconds):"
echo "  timeout 30 ./iot_parser_fuzztest --fuzz=IoTParserTest.FuzzMqttTopicParsing"
echo ""
echo "Press Ctrl+C to stop any running fuzz test."

cd .. 