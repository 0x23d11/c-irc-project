#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "common.h" // includes all necessary headers and constants

// Function declarations
int setup_server_socket(int port, int max_pending_connections);

#endif // SERVER_UTILS_H