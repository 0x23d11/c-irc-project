#include "client_handler.h"
#include "server_utils.h"

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
    char reply_buffer[BUFFER_SIZE + 100]; 
    ssize_t bytes_received;
    int client_index = -1; 
    int nickname_chosen = 0; 

    printf("Starting session for client fd %d. Waiting for nickname...\\n", client_socket_fd);

    // Loop for nickname acquisition
    while (!nickname_chosen) {
        snprintf(reply_buffer, sizeof(reply_buffer), "Welcome to the IRC server! Please enter your nickname: ");
        if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
            perror("send prompt for nickname failed");
            return; 
        }

        memset(nickname, 0, MAX_NICK_LENGTH);
        bytes_received = recv(client_socket_fd, nickname, MAX_NICK_LENGTH - 1, 0);

        if (bytes_received > 0) {
            nickname[bytes_received] = '\0';
            strip_newline(nickname);

            if (strlen(nickname) == 0) { 
                // Client sent an empty nickname string after stripping newline
                snprintf(reply_buffer, sizeof(reply_buffer), "Nickname cannot be empty. Please try again.\\n");
                if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                     perror("send empty nickname error failed"); return; }
                continue; // Ask for nickname again
            }
            // NOTE: The original code had a 'return' here for empty nickname, 
            // but for Option B (retry) we should 'continue' to ask again.
            // I have corrected this above.

            if (is_nickname_taken(nickname)) {
                snprintf(reply_buffer, sizeof(reply_buffer), "Nickname '%s' is already taken. Please choose another.\\n", nickname);
                 if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                     perror("send nickname taken error failed"); return; }
                continue; // Ask for nickname again
            }

            // Nickname is valid and not taken
            nickname_chosen = 1; // Set flag to exit loop

        } else if (bytes_received == 0) {
            printf("Client (%d) disconnected before choosing a nickname.\\n", client_socket_fd);
            return; 
        } else { 
            perror("recv nickname failed");
            return; 
        }
    } // End of nickname acquisition loop

    // ----- This is where the code after successful nickname choice should be -----
    // Add client to the clients array
    client_index = add_client(client_socket_fd, nickname);
    if (client_index == -1) {
        // If client addition fails, send error message and disconnect
        printf("Server full. Disconnecting client fd %d (%s)\\n", client_socket_fd, nickname);
        snprintf(reply_buffer, sizeof(reply_buffer), "Server is currently full. Please try again later. Disconnecting.\\n");
        send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0); // best effort send
        return; 
    }

    printf("Client fd %d set nickname to: %s, added at index %d\\n", client_socket_fd, nickname, client_index);

    // Send welcome message
    snprintf(reply_buffer, sizeof(reply_buffer), "Welcome, %s! You are connected.\\n", nickname);
    if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
        perror("send welcome message failed");
        // if send failed, client likely disconnected. We'll proceed to remove_client below.
    }
    // ----- End of the block that was duplicated -----

    // ----- This is the start of the main message loop -----
    printf("Starting message loop for %s (fd %d, idx %d)\\n", nickname, client_socket_fd, client_index);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; 
            strip_newline(buffer); 

            if (strlen(buffer) == 0) {
              continue;
            }

            printf("[%s] (fd %d, idx %d) sent: %s\\n", nickname, client_socket_fd, client_index, buffer);

            snprintf(reply_buffer, sizeof(reply_buffer), "[Server] %s: %s\\n", nickname, buffer);
            
            ssize_t bytes_sent = send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
            if (bytes_sent == -1) {
                perror("send failed");
                break; 
            } else if ((size_t)bytes_sent < strlen(reply_buffer)) {
                printf("Warning: partial send on fd %d.\\n", client_socket_fd);
            }

        } else if (bytes_received == 0) {
            printf("[%s] (fd %d, idx %d) disconnected.\\n", nickname, client_socket_fd, client_index);
            break;
        } else { 
            perror("recv failed");
            break;
        }
    } // ----- End of main message loop -----

    printf("Ending session for %s (fd %d, idx %d)\\n", nickname, client_socket_fd, client_index);

    if (client_index != -1) {
      remove_client(client_index);
    }
}