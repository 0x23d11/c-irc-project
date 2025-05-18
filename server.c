#include <stdio.h>      // for std IO
#include <stdlib.h> 
#include <sys/socket.h> // for socket programming functions and structures
#include <netinet/in.h> // for socket address and internet address structures
#include <unistd.h>     // for close() function
#include <arpa/inet.h>  // for inet_ntop() function
#include <string.h>     // for memset() function

#define PORT 6667                   // Standard IRC port
#define MAX_PENDING_CONNECTIONS 5   // Maximum number of pending connections
#define BUFFER_SIZE 1024            // Buffer size for messages

int main() {
    int server_fd, new_socket_fd;
    struct sockaddr_in server_address;
    struct sockaddr_storage client_address;
    socklen_t client_address_len;
    char client_ip_str[INET_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];
    char reply_buffer[BUFFER_SIZE + 100];
    ssize_t bytes_received;

    // 1) create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully\n");

    // 2) allow reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3) bind
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Socket bound to port %d\n", PORT);

    // 4) listen
    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", PORT);

    // 5) accept & communicate
    printf("Waiting for incoming connections...\n");
    while (1) {
        client_address_len = sizeof(client_address);
        new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (new_socket_fd < 0) {
            perror("accept failed");
            continue;
        }

        // log client IP
        if (client_address.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&client_address;
            inet_ntop(AF_INET, &s->sin_addr, client_ip_str, sizeof(client_ip_str));
            printf("Connection from %s:%d on fd %d\n", client_ip_str, ntohs(s->sin_port), new_socket_fd);
        } else {
            printf("Connection (family %d) on fd %d\n", client_address.ss_family, new_socket_fd);
        }

        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            bytes_received = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("Client (%d) sent: %s", new_socket_fd, buffer);

                // prepare reply
                snprintf(reply_buffer, sizeof(reply_buffer), "Server received: %s", buffer);
                // ensure newline
                if (reply_buffer[strlen(reply_buffer) - 1] != '\n' &&
                    strlen(reply_buffer) < sizeof(reply_buffer) - 1) {
                    strcat(reply_buffer, "\n");
                }  // <- added missing closing brace here

                ssize_t bytes_sent = send(new_socket_fd, reply_buffer, strlen(reply_buffer), 0);
                if (bytes_sent == -1) {
                    perror("send failed");
                    break;
                } else if (bytes_sent < strlen(reply_buffer)) {
                    printf("Warning: partial send on fd %d.\n", new_socket_fd);
                }

            } else if (bytes_received == 0) {
                printf("Client (%d) disconnected.\n", new_socket_fd);
                break;
            } else {
                perror("recv failed");
                break;
            }
        }

        close(new_socket_fd);
        printf("Closed fd %d, awaiting new connections...\n", new_socket_fd);
    }

    close(server_fd);
    return 0;
}
