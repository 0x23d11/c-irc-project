#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "common.h" // includes all necessary headers and constants

// Function declarations
int setup_server_socket(int port, int max_pending_connections);

void initialize_clients_array(); // initialize the global clients array

int add_client(int client_socket_fd, char *nickname);

void remove_client(int client_index); // remove a client from the global clients array

#endif // SERVER_UTILS_H