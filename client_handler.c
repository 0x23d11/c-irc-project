#include "client_handler.h"

// helper function to remove trailing newline characters
void strip_newline(char *str) {
  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}

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
  char nickname[MAX_NICK_LENGTH];
    char buffer[BUFFER_SIZE];
    char reply_buffer[BUFFER_SIZE + 100]; // Max reply size
    ssize_t bytes_received;

    printf("Starting session for client fd %d. Waiting for nickname...\n", client_socket_fd);

    // Send a prompt for nickname
    snprintf(reply_buffer, sizeof(reply_buffer), "Welcome to the IRC server! Please enter your nickname: ");
    send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);

    // Receive nickname
    memset(nickname, 0, MAX_NICK_LENGTH);
    bytes_received = recv(client_socket_fd, nickname, MAX_NICK_LENGTH - 1, 0);
    if (bytes_received > 0) {
      nickname[bytes_received] = '\0';
      strip_newline(nickname); // remove trailing newline
      
      if (strlen(nickname) == 0) { // Handle empty nickname submission
            printf("Client fd %d sent an empty nickname. Disconnecting.\n", client_socket_fd);
            snprintf(reply_buffer, sizeof(reply_buffer), "Nickname cannot be empty. Disconnecting.\\n");
            send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
            // No need to break or return here, the function will end, and the thread will close the socket.
            return; // Exit this handler function, socket will be closed by the thread
        }

        printf("Client fd %d set nickname to: %s\n", client_socket_fd, nickname);

        // Send welcome message
        snprintf(reply_buffer, sizeof(reply_buffer), "Welcome, %s! You are connected.\n", nickname);
        send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
    } else if (bytes_received == 0) {
        printf("Client (%d) disconnected.\n", client_socket_fd);
        return;
    } else {
      perror("recv failed");
      return;
    }

    printf("Starting message loop for %s client (fd %d)\n", nickname, client_socket_fd);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Ensure null termination
            strip_newline(buffer); // strip new lines from the regular message

            if (strlen(buffer) == 0) {
              // Ignore empty messages
              continue;
            }

            printf("[%s] (fd %d) sent: %s\\n", nickname, client_socket_fd, buffer);

            // Prepare reply
            snprintf(reply_buffer, sizeof(reply_buffer), "[Server] %s: %s\n", nickname, buffer);
            

            ssize_t bytes_sent = send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
            if (bytes_sent == -1) {
                perror("send failed");
                break; 
            } else if ((size_t)bytes_sent < strlen(reply_buffer)) {
                printf("Warning: partial send on fd %d.\n", client_socket_fd);
            }

        } else if (bytes_received == 0) {
            printf("[%s] (fd %d) disconnected.\n", nickname, client_socket_fd);
            break;
        } else { // bytes_received < 0
            perror("recv failed");
            break;
        }
    }
    printf("Ending session for %s (fd %d)\n", nickname, client_socket_fd);
    // client_socket_fd is closed by the calling thread in server.c after this function returns.
}