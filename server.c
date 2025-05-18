#include <stdio.h> // for std IO
#include <stdlib.h> 
#include <sys/socket.h> // for socket programming functions and structures
#include <netinet/in.h> // for socket address and internet address structures
#include <unistd.h> // for close() function
#include <arpa/inet.h> // for inet_ntop() function
#include <string.h> // for memset() function

#define PORT 6667 // Standard IRC port
#define MAX_PENDING_CONNECTIONS 5 // Maximum number of pending connections

int main() {
  int server_fd, new_socket_fd; // File descriptor for the server socket
  struct sockaddr_in server_address; // Struct to hold server address information
  struct sockaddr_storage client_address; // Struct to hold client address information
  socklen_t client_address_len; // Length of client address structure
  char client_ip_str[INET_ADDRSTRLEN]; // Buffer to store client IP address as string



  // Creating a socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) { // socket() returns -1 on error
    perror("socket failed"); // print error message
    exit(EXIT_FAILURE); // exit the program if socket fails
  }
  printf("Socket created successfully\n");

  // Initialize server_address structure to zero
  memset(&server_address, 0, sizeof(server_address));

  // Prepare the socket address struct
  server_address.sin_family = AF_INET; // Address family (IPv4)
  server_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available local IP address
  server_address.sin_port = htons(PORT); // Convert port to network byte order


  // Bind the socket to the address and port
  if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  printf("Socket bound to port %d\n", PORT);

  // Listen for incoming connections
  if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  printf("Server is listening on port %d\n", PORT);


  // Accept incoming connections
  printf("Waiting for incoming connections...\n");
  while(1) {
    client_address_len = sizeof(client_address);
    new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);

    if (new_socket_fd < 0) {
      perror("accept failed");
      continue;
    }

    // Get client IP address for logging
    if (client_address.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in *)&client_address;
      inet_ntop(AF_INET, &s->sin_addr, client_ip_str, sizeof(client_ip_str));
      printf("Connection accepter from %s:%d\n", client_ip_str, ntohs(s->sin_port));
    } else {
      printf("Connection accepter (address family: %d)\n", client_address.ss_family);

    }

    printf("New connection established on socket fd %d. Closing it for now.\n", new_socket_fd);
    close(new_socket_fd);
  }

  printf("Server shutting down for now.\n");
  close(server_fd);
  return 0;
}