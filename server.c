#include "common.h"
#include "server_utils.h"
#include "client_handler.h"
#include <pthread.h>

// Struct to pass arguments to the client handler thread
typedef struct {
    int client_socket_fd;
} client_thread_args_t;

// Function to be executed by each client thread
void *client_thread_handler(void *arg) {
    client_thread_args_t *thread_args = (client_thread_args_t *)arg;
    int client_fd = thread_args->client_socket_fd;

    printf("Client thread started for fd %d\n", client_fd);

    handle_client_session(client_fd); // handle the client session

    printf("Client thread finished for fd %d\n", client_fd);
    close(client_fd);
    free(thread_args);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_storage client_address; // Use sockaddr_storage for IPv4/IPv6 compatibility
    socklen_t client_address_len;

    // setup_server_socket is now defined in server_utils.c
    server_fd = setup_server_socket(PORT, MAX_PENDING_CONNECTIONS);

    printf("Waiting for incoming connections...\n");
    while (1) {
        client_address_len = sizeof(client_address);
        int new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        
        if (new_socket_fd < 0) {
            perror("accept failed");
            // If accept fails, we should continue to try and accept new connections,
            // rather than exiting the server.
            continue; 
        }

        // log_client_connection_info is now defined in client_handler.c
        log_client_connection_info(&client_address, client_address_len, new_socket_fd);
        
        // Prepare arguments for the client thread
        client_thread_args_t *thread_args = malloc(sizeof(client_thread_args_t));
        if (thread_args == NULL) {
            perror("Failed to allocate memory for thread args");
            close(new_socket_fd);
            continue;
        }

        thread_args->client_socket_fd = new_socket_fd;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread_handler, (void *)thread_args) != 0) {
            perror("Failed to create client thread");
            close(new_socket_fd);
            free(thread_args);
            continue;
        }
        
    }

    // This part of the code is typically only reached if the server is explicitly
    // signaled to shut down (e.g., by a signal handler not yet implemented).
    // In a simple Ctrl+C scenario, the program often terminates before reaching this.
    printf("Shutting down server.\n"); 
    close(server_fd); // Close the listening socket
    return 0;
}
