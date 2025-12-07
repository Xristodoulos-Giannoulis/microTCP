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

#include "microtcp.h"
#include "../utils/crc32.h"

microtcp_sock_t
microtcp_socket (int domain, int type, int protocol)
{
  microtcp_sock_t this_sock;
  int sock ;
  if (( sock = socket ( AF_INET , SOCK_DGRAM , IPPROTO_UDP )) == -1) {
    perror ( " SOCKET COULD NOT BE OPENED " );
    exit ( EXIT_FAILURE );
  }
  this_sock.sd = sock;
  this_sock.state = CLOSED;
  this_sock.init_win_size = MICROTCP_WIN_SIZE;
  this_sock.curr_win_size = MICROTCP_WIN_SIZE;
  this_sock.recvbuf = malloc(MICROTCP_RECVBUF_LEN);
  this_sock.buf_fill_level = 0;
  this_sock.cwnd = MICROTCP_INIT_CWND;
  this_sock.ssthresh = MICROTCP_INIT_SSTHRESH;
  this_sock.seq_number = 0;
  this_sock.ack_number = 0;
  this_sock.packets_send = 0;
  this_sock.packets_received = 0;
  this_sock.packets_lost = 0;
  this_sock.bytes_received = 0;
  this_sock.bytes_lost = 0;
  this_sock.bytes_send = 0;
  return this_sock;
  /* Your code here */

}

int
microtcp_bind (microtcp_sock_t *socket, const struct sockaddr *address,
               socklen_t address_len)
{
  //address_len kinda useless here, maybe because IPv6 and IPv4 have different sizes?
  if ( bind ( socket->sd , address , address_len ) == -1 ) {
    perror ( " BINDING FAILED " );
    exit ( EXIT_FAILURE );
  }
  /* Your code here */
}

int
microtcp_connect (microtcp_sock_t *socket, const struct sockaddr *address,
                  socklen_t address_len) //called by client, given the client's socket, and the destination
                  //(server) address (IP + port)
{
  if(socket->state != CLOSED) {
    return -1; //socket already used
  }
  uint32_t my_seq = rand();
  socket->seq_number = my_seq;
  /* Your code here XRISTOD*/

  microtcp_header_t header;
    memset(&header, 0, sizeof(header));

    header.seq_num = htonl(my_seq);
    header.ack_num = htonl(0);
    header.data_len = htonl(0);
    header.window = htons(1024);

    uint16_t flags = (1 << 13);
    header.control = htons(flags);

    header.checksum = crc32((const uint8_t *)&header, sizeof(header));

    ssize_t bytes_sent = sendto(socket->sd, &header, sizeof(header), 0, address, address_len);

    if (bytes_sent == -1) {
        return -1;
    }

    socket->state = SYN_SENT;
}

int
microtcp_accept (microtcp_sock_t *socket, struct sockaddr *address,
                 socklen_t address_len)
{
  /* Your code here STEF*/
}

int
microtcp_shutdown (microtcp_sock_t *socket, int how)
{
  /* Your code here */
}

ssize_t
microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length,
               int flags)
{
  /* Your code here */
}

ssize_t
microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags)
{
  /* Your code here */
}
