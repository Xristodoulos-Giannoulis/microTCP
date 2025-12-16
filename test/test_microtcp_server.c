/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "../lib/microtcp.h"

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

int main(void)
{
    microtcp_sock_t server_socket;
    microtcp_sock_t conn_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    char buffer[BUFFER_SIZE];

    printf("[Server] Starting...\n");

    server_socket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    printf("[Server] Socket created\n");

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    microtcp_bind(&server_socket,
                  (struct sockaddr *)&server_address,
                  sizeof(server_address));
    printf("[Server] Bound to port %d\n", SERVER_PORT);
    printf("[Server] Waiting for connection...\n");
    conn_socket.sd = microtcp_accept(&server_socket,
                                  (struct sockaddr *)&client_address,
                                  client_len);

    printf("[Server] Client connected\n");
    memset(buffer, 0, BUFFER_SIZE);
    microtcp_recv(&conn_socket, buffer, BUFFER_SIZE, 0);
    printf("[Server] Received: %s\n", buffer);

    char reply[] = "Hello Client!";
    microtcp_send(&conn_socket, reply, strlen(reply), 0);
    printf("[Server] Reply sent\n");

    microtcp_shutdown(&conn_socket, 0);
    printf("[Server] Connection closed\n");

    return 0;
}
