#include "fuzztest/fuzztest.h"
#include "gtest/gtest.h"
#include "iot_parser.h"
#include <string>
#include <cstring>

// ========================================
// UNIT TESTS (Basic functionality)
// ========================================

TEST(IoTParserTest, ValidMqttParsing) {
    mqtt_message_t msg = {0};
    parse_mqtt_topic("mqtt/sensors/temp {\"temperature\":25.5}", &msg);
    EXPECT_STREQ(msg.topic, "/sensors/temp");
    EXPECT_STREQ(msg.payload, "{\"temperature\":25.5}");
}

TEST(IoTParserTest, ValidJsonExtraction) {
    char value[MAX_VALUE_SIZE];
    int result = extract_json_value("{\"device_id\":\"sensor1\",\"temp\":23}", "device_id", value);
    EXPECT_GT(result, 0);
    EXPECT_STREQ(value, "sensor1");
}

TEST(IoTParserTest, ValidHttpParsing) {
    http_message_t msg = {0};
    parse_http_request("GET /api/sensors {\"action\":\"read\"}", &msg);
    EXPECT_STREQ(msg.method, "GET");
    EXPECT_STREQ(msg.path, "/api/sensors");
    EXPECT_STREQ(msg.body, "{\"action\":\"read\"}");
}

// ========================================
// FUZZ TESTS (Property-based testing)
// ========================================

// Fuzz Test 1: MQTT Topic Parsing - Target buffer overflow vulnerability
void FuzzMqttTopicParsing(const std::string& input) {
    // Skip empty inputs and ensure minimum format
    if (input.length() < 4) return;
    
    mqtt_message_t msg = {0};
    
    // This should not crash even with malformed input
    // The original code has buffer overflow in strcpy - sanitizers will catch it
    parse_mqtt_topic(input.c_str(), &msg);
    
    // Properties that should always hold:
    // 1. Topic should be null-terminated (if we don't crash)
    EXPECT_LE(strlen(msg.topic), MAX_TOPIC_SIZE - 1);
    // 2. Payload should be null-terminated (if we don't crash)  
    EXPECT_LE(strlen(msg.payload), MAX_BUFFER_SIZE - 1);
}

FUZZ_TEST(IoTParserTest, FuzzMqttTopicParsing)
    .WithDomains(fuzztest::Arbitrary<std::string>()
                     .WithMinSize(4)
                     .WithMaxSize(512));

// Fuzz Test 2: JSON Value Extraction - Target integer overflow and bounds issues
void FuzzJsonExtraction(const std::string& json, const std::string& key) {
    // Skip inputs that are too large or empty
    if (json.empty() || key.empty() || json.length() > 1000 || key.length() > 30) {
        return;
    }
    
    char value[MAX_VALUE_SIZE];
    memset(value, 0, sizeof(value));
    
    // This should not crash with any input
    int result = extract_json_value(json.c_str(), key.c_str(), value);
    
    // Properties:
    // 1. Result should be non-negative
    EXPECT_GE(result, 0);
    // 2. If successful, value should be null-terminated
    if (result > 0) {
        EXPECT_LE(strlen(value), MAX_VALUE_SIZE - 1);
    }
}

FUZZ_TEST(IoTParserTest, FuzzJsonExtraction)
    .WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(500),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(30));

// Fuzz Test 3: HTTP Request Parsing - Target use-after-free vulnerability
void FuzzHttpRequestParsing(const std::string& input) {
    if (input.empty() || input.length() > 1000) return;
    
    http_message_t msg = {0};
    
    // This targets the use-after-free bug in parse_http_request
    parse_http_request(input.c_str(), &msg);
    
    // Properties:
    // 1. Method should be null-terminated
    EXPECT_LE(strlen(msg.method), sizeof(msg.method) - 1);
    // 2. Path should be null-terminated
    EXPECT_LE(strlen(msg.path), MAX_PATH_SIZE - 1);
    // 3. Body should be null-terminated
    EXPECT_LE(strlen(msg.body), MAX_BUFFER_SIZE - 1);
}

FUZZ_TEST(IoTParserTest, FuzzHttpRequestParsing)
    .WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(512));

// Fuzz Test 4: MQTT Topic with Structured Input (More targeted fuzzing)
void FuzzMqttStructured(const std::string& topic, const std::string& payload) {
    if (topic.empty() || payload.empty()) return;
    
    // Construct MQTT-like input
    std::string mqtt_input = "mqtt" + topic + " " + payload;
    if (mqtt_input.length() > 1000) return;
    
    mqtt_message_t msg = {0};
    parse_mqtt_topic(mqtt_input.c_str(), &msg);
    
    // The topic extraction should not exceed buffer bounds
    EXPECT_LE(strlen(msg.topic), MAX_TOPIC_SIZE - 1);
    EXPECT_LE(strlen(msg.payload), MAX_BUFFER_SIZE - 1);
}

FUZZ_TEST(IoTParserTest, FuzzMqttStructured)
    .WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(100),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(200));

// Fuzz Test 5: JSON with Various Key-Value Patterns
void FuzzJsonKeyValue(const std::string& prefix, const std::string& key, 
                      const std::string& value, const std::string& suffix) {
    if (key.empty() || key.length() > 20) return;
    
    // Build JSON-like structure
    std::string json = prefix + "\"" + key + "\":\"" + value + "\"" + suffix;
    if (json.length() > 800) return;
    
    char extracted[MAX_VALUE_SIZE];
    memset(extracted, 0, sizeof(extracted));
    
    int result = extract_json_value(json.c_str(), key.c_str(), extracted);
    
    // If extraction succeeds, the result should be reasonable
    if (result > 0) {
        EXPECT_LE(strlen(extracted), MAX_VALUE_SIZE - 1);
        EXPECT_GT(strlen(extracted), 0);
    }
}

FUZZ_TEST(IoTParserTest, FuzzJsonKeyValue)
    .WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(50),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(20),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(50),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(50));

// Fuzz Test 6: HTTP Methods and Paths (Target format string bug)
void FuzzHttpMethodPath(const std::string& method, const std::string& path, 
                        const std::string& body) {
    if (method.empty() || path.empty()) return;
    
    // Construct HTTP-like request
    std::string http_request = method + " " + path + " " + body;
    if (http_request.length() > 800) return;
    
    http_message_t msg = {0};
    
    // This should not crash even with format string characters in input
    parse_http_request(http_request.c_str(), &msg);
    
    // Verify bounds
    EXPECT_LE(strlen(msg.method), sizeof(msg.method) - 1);
    EXPECT_LE(strlen(msg.path), MAX_PATH_SIZE - 1);
    EXPECT_LE(strlen(msg.body), MAX_BUFFER_SIZE - 1);
}

FUZZ_TEST(IoTParserTest, FuzzHttpMethodPath)
    .WithDomains(fuzztest::Arbitrary<std::string>().WithMaxSize(10),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(60),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(200));

// Fuzz Test 7: Edge Cases - Empty and boundary inputs
void FuzzEdgeCases(int input_type, const std::string& data) {
    if (data.length() > 500) return;
    
    switch (input_type % 3) {
        case 0: {
            // Test MQTT with edge case data
            mqtt_message_t msg = {0};
            parse_mqtt_topic(data.c_str(), &msg);
            break;
        }
        case 1: {
            // Test JSON extraction with edge case
            char value[MAX_VALUE_SIZE];
            extract_json_value(data.c_str(), "test", value);
            break;
        }
        case 2: {
            // Test HTTP with edge case data
            http_message_t msg = {0};
            parse_http_request(data.c_str(), &msg);
            break;
        }
    }
}

FUZZ_TEST(IoTParserTest, FuzzEdgeCases)
    .WithDomains(fuzztest::InRange(0, 1000),
                 fuzztest::Arbitrary<std::string>().WithMaxSize(300)); 