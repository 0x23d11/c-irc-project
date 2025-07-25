#include "client_handler.h"
#include "server_utils.h"
#include <pthread.h>


extern pthread_mutex_t clients_mutex;
extern client_info_t clients[MAX_CLIENTS];

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

    printf("Starting session for client fd %d. Waiting for nickname...\n", client_socket_fd);

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
                snprintf(reply_buffer, sizeof(reply_buffer), "Nickname cannot be empty. Please try again.\n");
                if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                     perror("send empty nickname error failed"); return; }
                continue; // Ask for nickname again
            }
            // NOTE: The original code had a 'return' here for empty nickname, 
            // but for Option B (retry) we should 'continue' to ask again.
            // I have corrected this above.

            if (is_nickname_taken(nickname)) {
                snprintf(reply_buffer, sizeof(reply_buffer), "Nickname '%s' is already taken. Please choose another.\n", nickname);
                 if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                     perror("send nickname taken error failed"); return; }
                continue; // Ask for nickname again
            }

            // Nickname is valid and not taken
            nickname_chosen = 1; // Set flag to exit loop

        } else if (bytes_received == 0) {
            printf("Client (%d) disconnected before choosing a nickname.\n", client_socket_fd);
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
        printf("Server full. Disconnecting client fd %d (%s)\n", client_socket_fd, nickname);
        snprintf(reply_buffer, sizeof(reply_buffer), "Server is currently full. Please try again later. Disconnecting.\n");
        send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0); // best effort send
        return; 
    }

    printf("Client fd %d set nickname to: %s, added at index %d\n", client_socket_fd, nickname, client_index);

    // Send welcome message
    snprintf(reply_buffer, sizeof(reply_buffer), "Welcome, %s! You are connected.\n", nickname);
    if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
        perror("send welcome message failed");
        // if send failed, client likely disconnected. We'll proceed to remove_client below.
    }
    // ----- End of the block that was duplicated -----

    // ----- This is the start of the main message loop -----
    printf("Starting message loop for %s (fd %d, idx %d)\n", nickname, client_socket_fd, client_index);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; 
            strip_newline(buffer); 

            if (strlen(buffer) == 0) {
              continue;
            }

            // check if the message is a command
            if (buffer[0] == '/') {
                printf("[%s] (fd %d, idx %d) sent COMMAND: %s\n", nickname, client_socket_fd, client_index, buffer);

                char *command = NULL;
                char *saveptr = NULL;
                char *args = NULL;

                char command_buffer[BUFFER_SIZE];
                strncpy(command_buffer, buffer, BUFFER_SIZE -1);
                command_buffer[BUFFER_SIZE-1] = '\0';


                command = strtok_r(command_buffer + 1, " ", &saveptr); 
                if (saveptr != NULL && *saveptr != '\0') { // Check if there is anything after the first space
                    args = saveptr; // The rest of the string is arguments
                }


                if (command != NULL) {
                    if (strcmp(command, "quit") == 0) {
                        snprintf(reply_buffer, sizeof(reply_buffer), "Goodbye, %s! Disconnecting.\n", nickname);
                        send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0); // Best effort send
                        printf("[%s] (fd %d, idx %d) issued /quit command. Disconnecting.\n", nickname, client_socket_fd, client_index);
                        break; // Exit the message loop, client will be cleaned up
                    } else if (strcmp(command, "nick") == 0) {
                        if (args == NULL || strlen(args) == 0) {
                            snprintf(reply_buffer, sizeof(reply_buffer), "Usage: /nick <new_nickname>\n");
                        } else {
                            char new_nick[MAX_NICK_LENGTH];
                            strncpy(new_nick, args, MAX_NICK_LENGTH - 1);
                            new_nick[MAX_NICK_LENGTH - 1] = '\0';
                            strip_newline(new_nick); // Clean it up

                            if (strlen(new_nick) == 0) {
                                snprintf(reply_buffer, sizeof(reply_buffer), "New nickname cannot be empty.\n");
                            } else if (strlen(new_nick) >= MAX_NICK_LENGTH) {
                                snprintf(reply_buffer, sizeof(reply_buffer), "New nickname is too long (max %d chars).\n", MAX_NICK_LENGTH - 1);
                            } else if (strcmp(nickname, new_nick) == 0) {
                                snprintf(reply_buffer, sizeof(reply_buffer), "You are already known as %s.\n", new_nick);
                            } else if (is_nickname_taken(new_nick)) {
                                snprintf(reply_buffer, sizeof(reply_buffer), "Nickname '%s' is already taken.\n", new_nick);
                            } else {
                                // Valid new nickname, proceed to update
                                char old_nickname_log[MAX_NICK_LENGTH];
                                strncpy(old_nickname_log, nickname, MAX_NICK_LENGTH); // For logging

                                pthread_mutex_lock(&clients_mutex);
                                strncpy(clients[client_index].nickname, new_nick, MAX_NICK_LENGTH - 1);
                                clients[client_index].nickname[MAX_NICK_LENGTH - 1] = '\0';
                                pthread_mutex_unlock(&clients_mutex);

                                // Update local nickname variable as well
                                strncpy(nickname, new_nick, MAX_NICK_LENGTH -1);
                                nickname[MAX_NICK_LENGTH-1] = '\0';

                                snprintf(reply_buffer, sizeof(reply_buffer), "Your nickname has been changed to %s.\n", nickname);
                                printf("[%s] (fd %d, idx %d) changed nickname to %s.\n", old_nickname_log, client_socket_fd, client_index, nickname);
                                // TODO: Broadcast nickname change to other users/channels
                            }
                        }
                        if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                            perror("send nick response failed");
                        }
                    } 
                    // Add other command handlers here later (e.g., /nick, /help)
                    // else if (strcmp(command, "nick") == 0) { /* ... */ }
                    else {
                        snprintf(reply_buffer, sizeof(reply_buffer), "Unknown command: /%s\n", command);
                        if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                            perror("send unknown command failed");
                            // If send fails, client might have disconnected.
                            // The loop will break on next recv or if quit was issued.
                        }
                    }
                } else {
                    // Empty command like "/" or "/ "
                    snprintf(reply_buffer, sizeof(reply_buffer), "Empty command. Type /help for available commands.\n");
                     if (send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0) == -1) {
                        perror("send empty command failed");
                    }
                }
                
                // TODO: Parse command and arguments
                // For now, just acknowledge command received, no reply to client yet for commands
                // Example: /nick new_nickname
                //          /quit
                //          /help
                
                // Placeholder for sending a message back to client for unknown command
                // snprintf(reply_buffer, sizeof(reply_buffer), "Unknown command: %s\\n", buffer);
                // send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
            } else {
                // if the message is not a command, it is a message
                printf("[%s] (fd %d, idx %d) sent MESSAGE: %s\n", nickname, client_socket_fd, client_index, buffer);


                snprintf(reply_buffer, sizeof(reply_buffer), "[Server] %s: %s\n", nickname, buffer);
                
                ssize_t bytes_sent = send(client_socket_fd, reply_buffer, strlen(reply_buffer), 0);
                if (bytes_sent == -1) {
                    perror("send failed");
                    break; 
                } else if ((size_t)bytes_sent < strlen(reply_buffer)) {
                    printf("Warning: partial send on fd %d.\n", client_socket_fd);
                }
            }

        } else if (bytes_received == 0) {
            printf("[%s] (fd %d, idx %d) disconnected.\n", nickname, client_socket_fd, client_index);
            break;
        } else { 
            perror("recv failed");
            break;
        }
    } // ----- End of main message loop -----

    printf("Ending session for %s (fd %d, idx %d)\n", nickname, client_socket_fd, client_index);

    if (client_index != -1) {
      remove_client(client_index);
    }
}