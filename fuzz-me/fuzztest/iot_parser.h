#ifndef IOT_PARSER_H
#define IOT_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

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

// Function declarations
void parse_mqtt_topic(const char* input, mqtt_message_t* msg);
int extract_json_value(const char* json, const char* key, char* value);
void parse_http_request(const char* input, http_message_t* msg);
void print_mqtt_analysis(const mqtt_message_t* msg);
void print_http_analysis(const http_message_t* msg);

#ifdef __cplusplus
}
#endif

#endif // IOT_PARSER_H 