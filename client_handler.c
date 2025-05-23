#include "client_handler.h"

void log_client_connection_info(struct sockaddr_storage *client_address, socklen_t client_address_len, int client_fd) {
  char client_ip_str[INET_ADDRSTRLEN];

  if (client_address->ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)client_address;
    inet_ntop(AF_INET, &s->sin_addr, client_ip_str, sizeof(client_ip_str));
    printf("Client connected from %s:%d\n", client_ip_str, ntohs(s->sin_port));
  } else if (client_address->ss_family == AF_INET6) {
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)client_address;
    inet_ntop(AF_INET6, &s->sin6_addr, client_ip_str, sizeof(client_ip_str));
    printf("Client connected from %s:%d\n", client_ip_str, ntohs(s->sin6_port));
  } else {
    printf("Client connected from unknown address\n");
  }
}

void handle_client_session(int client_socket_fd) {
    char buffer[BUFFER_SIZE];
    char reply_buffer[BUFFER_SIZE + 100]; // Max reply size
    ssize_t bytes_received;

    printf("Starting session for client fd %d\n", client_socket_fd);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Ensure null termination
            printf("Client (%d) sent: %s", client_socket_fd, buffer);

            // Prepare reply
            snprintf(reply_buffer, sizeof(reply_buffer), "Server received: %s", buffer);
            
            size_t reply_len = strlen(reply_buffer);
            if (reply_len > 0 && reply_buffer[reply_len - 1] != '\n' && reply_len < sizeof(reply_buffer) - 1) {
                reply_buffer[reply_len] = '\n';
                reply_buffer[reply_len + 1] = '\0';
            }

            ssize_t bytes_sent = send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
            if (bytes_sent == -1) {
                perror("send failed");
                break; 
            } else if ((size_t)bytes_sent < strlen(reply_buffer)) {
                printf("Warning: partial send on fd %d.\n", client_socket_fd);
            }

        } else if (bytes_received == 0) {
            printf("Client (%d) disconnected.\n", client_socket_fd);
            break;
        } else { // bytes_received < 0
            perror("recv failed");
            break;
        }
    }
    printf("Ending session for client fd %d\n", client_socket_fd);
    // client_socket_fd is closed by the calling thread in server.c after this function returns.
}