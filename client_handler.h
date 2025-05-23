#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "common.h" // Includes all necessary system headers and constants

// Function declarations
void log_client_connection_info(struct sockaddr_storage *client_address, socklen_t client_address_len, int client_fd);
void handle_client_session(int client_socket_fd);

#endif // CLIENT_HANDLER_H