/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2022  Manolis Surligas <surligas@gmail.com>
 */

#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit, EXIT_FAILURE
#include <string.h>     // For memset
#include <arpa/inet.h>  // For htons, inet_pton
#include <sys/socket.h> // For AF_INET

/*
 * The microtcp library header must be included.
 * The path is relative to this file's location in `test/`.
 */
#include "../lib/microtcp.h"

// Define a placeholder IP and Port for the server.
// These would typically be read from command-line arguments.
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
    microtcp_sock_t client_socket;
    
    /*
     * We use 'struct sockaddr_in' for an IPv4 address because it has the
     * necessary fields (.sin_family, .sin_port, .sin_addr) to hold the
     * address information.
     */
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;

    printf("[Client] Starting...\n");

    // Create a microTCP socket.
    // The arguments match the standard socket() call.
    client_socket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    //maybe add an error checking to see if it happened? not yet.
    printf("[Client] microTCP socket created.\n");

    // Zero out the server address structure.
    memset(&server_addr, 0, sizeof(server_addr));

    // Configure the server address details (IPv4? for now idk)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr_len = sizeof(server_addr);

    // Convert the IP address from text to binary format.
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[Client] Invalid server address");
        exit(EXIT_FAILURE);
    }

    //connect expects a generic sockaddr pointer, so we cast our sockaddr_in to that
    if (microtcp_connect(&client_socket, (struct sockaddr *)&server_addr, server_addr_len) < 0) { //connect returns -1 on failure
        perror("[Client] microTCP connection failed");
        exit(EXIT_FAILURE);
    }

    printf("[Client] Connection established with server at %s:%d\n", SERVER_IP, SERVER_PORT);

    char message[] = "Hello Server!";
    int sent_check = microtcp_send(&client_socket, message, strlen(message), 0);
    //maybe an if check to see if sent_check is -1 (error)
    printf("[Client] Sent message to server: %s\n", message);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int received_check = microtcp_recv(&client_socket, buffer, BUFFER_SIZE, 0);

    // Close the connection.
    if (microtcp_shutdown(&client_socket, 0) < 0) { //assuming that microtcp_shutdown returns -1 on failure
        perror("[Client] microTCP shutdown failed");
    } else {
        printf("[Client] Connection successfully closed.\n");
    }

    return 0;
}