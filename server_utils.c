#include "server_utils.h"

// Define for setup_server_socket
int setup_server_socket(int port, int max_pending_connections) {
  int server_fd;
  struct sockaddr_in server_address;

  // 1) create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket failed");
    return -1;
  }
  printf("Socket created successfully\n");

  // allow reuse
  int opt = 1;

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 3) bind socket to address
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  printf("Socket bound to port %d\n", port);

  // 4) listen for connections
  if (listen(server_fd, max_pending_connections) < 0) {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  printf("Server is listening on port %d\n", port);

  return server_fd;
}

