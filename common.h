#ifndef COMMON_H
#define COMMON_H

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
#define MAX_NICK_LENGTH 32          // Maximum length of a nickname

#endif // COMMON_H