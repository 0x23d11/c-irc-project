#include "common.h"
#include "server_utils.h"
#include "client_handler.h"
// We will add #include <pthread.h> here when implementing threads

int main() {
    int server_fd, new_socket_fd;
    struct sockaddr_storage client_address; // Use sockaddr_storage for IPv4/IPv6 compatibility
    socklen_t client_address_len;

    // setup_server_socket is now defined in server_utils.c
    server_fd = setup_server_socket(PORT, MAX_PENDING_CONNECTIONS);

    printf("Waiting for incoming connections...\n");
    while (1) {
        client_address_len = sizeof(client_address);
        new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        
        if (new_socket_fd < 0) {
            perror("accept failed");
            // If accept fails, we should continue to try and accept new connections,
            // rather than exiting the server.
            continue; 
        }

        // log_client_connection_info is now defined in client_handler.c
        log_client_connection_info(&client_address, client_address_len, new_socket_fd);
        
        // handle_client_session is now defined in client_handler.c
        // For now, we handle one client at a time.
        // This call will block until the client session ends.
        // Later, we will spawn a thread here for handle_client_session.
        handle_client_session(new_socket_fd);

        // The client socket should be closed after its session ends.
        // When we move to threads, this close operation will be done within 
        // the thread managing the client session, right before the thread exits.
        // For now, it's correct to close it here after the blocking handle_client_session call.
        close(new_socket_fd); 
        printf("Closed fd %d, awaiting new connections...\n", new_socket_fd);
    }

    // This part of the code is typically only reached if the server is explicitly
    // signaled to shut down (e.g., by a signal handler not yet implemented).
    // In a simple Ctrl+C scenario, the program often terminates before reaching this.
    printf("Shutting down server.\n"); 
    close(server_fd); // Close the listening socket
    return 0;
}
