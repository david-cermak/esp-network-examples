#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8080
#define CHUNK_SIZE 100

typedef struct {
    int client_fd;
    char *file_data;
    size_t file_size;
} client_args_t;

void *handle_client(void *arg) {
    client_args_t *cargs = (client_args_t *)arg;
    int fd = cargs->client_fd;
    char *data = cargs->file_data;
    size_t size = cargs->file_size;

    // Send basic HTTP header
    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/html\r\n"
             "\r\n",
             size);
    send(fd, header, strlen(header), 0);

    // Send the file in chunks
    for (size_t offset = 0; offset < size; offset += CHUNK_SIZE) {
        size_t chunk = (size - offset > CHUNK_SIZE) ? CHUNK_SIZE : (size - offset);
        send(fd, data + offset, chunk, 0);
        printf("[Thread %ld] Sent %zu bytes\n", pthread_self(), chunk);
        // usleep(20000); // 200ms
        sleep(1); // 1 second
    }

    close(fd);
    free(cargs);
    return NULL;
}

// int main() {
void async_server(void) {
    int server_fd;
    int client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
#if 0


    // Read file into memory
    const char *filename = "index.html";
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *file_data = malloc(fsize);
    fread(file_data, 1, fsize, fp);
    fclose(fp);
#else
    extern const uint8_t index_html[]   asm("_binary_index_html_start");
    extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
    size_t fsize = index_html_end - index_html;
    char *file_data = malloc(fsize);
    memcpy(file_data, index_html, fsize);
#endif
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server listening on port %d...\n", PORT);

    while ((client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen)) >= 0) {
        client_args_t *cargs = malloc(sizeof(client_args_t));
        cargs->client_fd = client_fd;
        cargs->file_data = file_data;
        cargs->file_size = fsize;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, cargs);
        pthread_detach(tid);
    }

    free(file_data);
    // close(server_fd);
    // return 0;
}
