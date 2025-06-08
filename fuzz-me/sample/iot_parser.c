#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 256
#define MAX_TOPIC_SIZE 64
#define MAX_PATH_SIZE 64
#define MAX_KEY_SIZE 32
#define MAX_VALUE_SIZE 64

typedef struct {
    char topic[MAX_TOPIC_SIZE];
    char payload[MAX_BUFFER_SIZE];
} mqtt_message_t;

typedef struct {
    char method[8];
    char path[MAX_PATH_SIZE];
    char body[MAX_BUFFER_SIZE];
} http_message_t;

// Function prototypes
void parse_mqtt_topic(const char* input, mqtt_message_t* msg);
int extract_json_value(const char* json, const char* key, char* value);
void parse_http_request(const char* input, http_message_t* msg);
void print_mqtt_analysis(const mqtt_message_t* msg);
void print_http_analysis(const http_message_t* msg);

// Subtle bug #1: No bounds checking on strcpy
void parse_mqtt_topic(const char* input, mqtt_message_t* msg) {
    const char* space_pos = strchr(input + 4, ' '); // Skip "mqtt"
    if (space_pos) {
        int topic_len = space_pos - (input + 4);
        strncpy(msg->topic, input + 4, topic_len); // Off-by-one potential
        msg->topic[topic_len] = '\0';
        strcpy(msg->payload, space_pos + 1); // Buffer overflow potential
    }
}

// Subtle bug #2: Integer overflow in length calculation
int extract_json_value(const char* json, const char* key, char* value) {
    char search_key[MAX_KEY_SIZE + 4];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    const char* key_pos = strstr(json, search_key);
    if (!key_pos) return 0;
    
    const char* value_start = key_pos + strlen(search_key);
    while (isspace(*value_start)) value_start++;
    
    if (*value_start == '"') {
        value_start++;
        const char* value_end = strchr(value_start, '"');
        if (value_end) {
            int len = value_end - value_start;
            // Subtle bug: No bounds checking
            strncpy(value, value_start, len);
            value[len] = '\0';
            return len;
        }
    } else {
        // Handle numeric values
        char* end_ptr;
        long num = strtol(value_start, &end_ptr, 10);
        // Subtle bug: Integer overflow not checked
        sprintf(value, "%ld", num);
        return end_ptr - value_start;
    }
    return 0;
}

// Subtle bug #3: Use-after-free potential
void parse_http_request(const char* input, http_message_t* msg) {
    char* input_copy = malloc(strlen(input) + 1);
    strcpy(input_copy, input);
    
    char* method_end = strchr(input_copy, ' ');
    if (method_end) {
        *method_end = '\0';
        strcpy(msg->method, input_copy);
        
        char* path_start = method_end + 1;
        char* path_end = strchr(path_start, ' ');
        if (path_end) {
            *path_end = '\0';
            // Subtle bug: No length check
            strcpy(msg->path, path_start);
            
            char* body_start = path_end + 1;
            strcpy(msg->body, body_start);
        }
    }
    
    free(input_copy);
    // Potential use of freed memory if we access input_copy later
}

void print_mqtt_analysis(const mqtt_message_t* msg) {
    printf("=== MQTT Message Analysis ===\n");
    printf("Topic: %s\n", msg->topic);
    printf("Payload: %s\n", msg->payload);
    
    // Extract some JSON values
    char device_id[MAX_VALUE_SIZE] = {0};
    char temperature[MAX_VALUE_SIZE] = {0};
    char status[MAX_VALUE_SIZE] = {0};
    
    if (extract_json_value(msg->payload, "device_id", device_id)) {
        printf("Device ID: %s\n", device_id);
    }
    if (extract_json_value(msg->payload, "temperature", temperature)) {
        printf("Temperature: %s\n", temperature);
    }
    if (extract_json_value(msg->payload, "status", status)) {
        printf("Status: %s\n", status);
    }
    printf("\n");
}

void print_http_analysis(const http_message_t* msg) {
    printf("=== HTTP Request Analysis ===\n");
    printf("Method: %s\n", msg->method);
    printf("Path: %s\n", msg->path);
    printf("Body: %s\n", msg->body);
    
    // Subtle bug #4: Format string vulnerability (INTENTIONAL - for fuzzing demo)
    char error_msg[100];
    if (strlen(msg->method) == 0) {
        sprintf(error_msg, "Warning: Empty method in request: %s", msg->path);
        printf(error_msg); // INTENTIONAL BUG: Should use printf("%s", error_msg)
    }
    
    // Extract JSON from body
    char action[MAX_VALUE_SIZE] = {0};
    char value[MAX_VALUE_SIZE] = {0};
    
    if (extract_json_value(msg->body, "action", action)) {
        printf("Action: %s\n", action);
    }
    if (extract_json_value(msg->body, "value", value)) {
        printf("Value: %s\n", value);
    }
    printf("\n");
}

int main(void) {
    char input[MAX_BUFFER_SIZE * 2];
    
    printf("IoT Network Message Parser\n");
    printf("Supported formats:\n");
    printf("  MQTT: mqtt/topic/path {\"key\":\"value\"}\n");
    printf("  HTTP: GET /path {\"key\":\"value\"}\n");
    printf("  HTTP: POST /path {\"key\":\"value\"}\n");
    printf("Enter message: ");
    
    if (fgets(input, sizeof(input), stdin)) {
        // Remove newline
        input[strcspn(input, "\n")] = '\0';
        
        if (strncmp(input, "mqtt", 4) == 0) {
            mqtt_message_t mqtt_msg = {0};
            parse_mqtt_topic(input, &mqtt_msg);
            print_mqtt_analysis(&mqtt_msg);
        } else if (strncmp(input, "GET", 3) == 0 || strncmp(input, "POST", 4) == 0) {
            http_message_t http_msg = {0};
            parse_http_request(input, &http_msg);
            print_http_analysis(&http_msg);
        } else {
            // Subtle bug #5: Potential null pointer dereference
            char* unknown_format = NULL;
            if (strlen(input) > 10) {
                unknown_format = malloc(20);
                strcpy(unknown_format, "Unknown format");
            }
            printf("Error: %s\n", unknown_format); // Potential NULL deref
            free(unknown_format);
        }
    }
    
    return 0;
} 