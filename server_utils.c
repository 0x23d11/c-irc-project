#include "server_utils.h"
#include <pthread.h>

extern client_info_t clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

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

void initialize_clients_array() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].active = 0;
    clients[i].client_socket_fd = -1; // Good practice to initialize to -1 to indicate not in use
    memset(clients[i].nickname, 0, MAX_NICK_LENGTH); // Initialize nickname to empty string
  }
  printf("Clients array initialized. Max clients: %d\n", MAX_CLIENTS);
}

int add_client(int client_socket_fd, char *nickname) {
  pthread_mutex_lock(&clients_mutex); // lock the mutex to prevent race conditions

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active == 0) { // find an inactive slot
      clients[i].active = 1;
      clients[i].client_socket_fd = client_socket_fd;
      strncpy(clients[i].nickname, nickname, MAX_NICK_LENGTH - 1);
      clients[i].nickname[MAX_NICK_LENGTH - 1] = '\0'; // ensure null termination
      pthread_mutex_unlock(&clients_mutex); // unlock the mutex
      printf("Client (fd %d) added with nickname: %s\n", client_socket_fd, nickname);
      return i; // success
    }
  }

  pthread_mutex_unlock(&clients_mutex); // unlock the mutex
  printf("Failed to add client (fd %d) with nickname: %s. Max clients reached.\n", client_socket_fd, nickname);
  return -1; // failure
}


void remove_client(int client_index) {
  if (client_index < 0 || client_index >= MAX_CLIENTS) {
    fprintf(stderr, "Error: Attempted to remove client with invalid index: %d\n", client_index);
    return;
  }

  pthread_mutex_lock(&clients_mutex); // lock the mutex to prevent race conditions

  if(clients[client_index].active == 1) {
    char removed_nickname[MAX_NICK_LENGTH];
    strncpy(removed_nickname, clients[client_index].nickname, MAX_NICK_LENGTH);
    removed_nickname[MAX_NICK_LENGTH - 1] = '\0'; // ensure null termination
    clients[client_index].active = 0;
    clients[client_index].client_socket_fd = -1;
    memset(clients[client_index].nickname, 0, MAX_NICK_LENGTH);
    
    printf("Client (idx %d) with nickname %s removed.\n", client_index, removed_nickname);
  } else {
    fprintf(stderr, "Warning: Attempted to remove client from slot %d, but it was already inactive.\n", client_index);
  }

  pthread_mutex_unlock(&clients_mutex); // unlock the mutex 
}