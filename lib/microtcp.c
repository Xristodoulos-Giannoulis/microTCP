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
  int sock;
  if ((sock = socket ( AF_INET , SOCK_DGRAM , IPPROTO_UDP )) == -1) {
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

  header.seq_num = my_seq;
  header.ack_num = 0;
  header.data_len = 0;
  header.window = MICROTCP_WIN_SIZE; 

  // Setting syn=1, Bit 14
  header.control = (1 << 14 );

  // Calculate checksum on the raw bytes as they sit in memory
  header.checksum = crc32((const uint8_t *)&header, sizeof(header));

  ssize_t bytes_sent = sendto(socket->sd, &header, sizeof(header), 0, address, address_len);

  if (bytes_sent == -1) {
      return -1;
  }

  socket->state = SYN_SENT;

  microtcp_header_t recv_header;
  struct sockaddr_in sender_addr;
  socklen_t sender_len = sizeof(sender_addr);

  while (1) {
      ssize_t bytes = recvfrom(socket->sd, &recv_header, sizeof(recv_header), 0,
      (struct sockaddr *)&sender_addr, &sender_len);

      //SOS TO CHECK THIS OUT 
      //!!
      //if (bytes < sizeof(microtcp_header_t)) continue;
      //!! 
      uint32_t received_csum = recv_header.checksum;
      recv_header.checksum = 0; // we have to make the check sum field zero, before comparing
      //since it was zeroed out before checksum was added to the header.
      if (crc32((const uint8_t *)&recv_header, sizeof(recv_header)) != received_csum) continue;

      uint16_t expected_flags = (1 << 14) | (1 << 12); // SYN + ACK
      if ((recv_header.control & expected_flags) != expected_flags) continue;

      if (recv_header.ack_num != my_seq + 1) continue;

      break; //If all checks are passed, we reached this point, we leave the loop
  }

  // Here we update seq,ack variables of the socket
  // and send the last packet, (3rd of the handshake)
  socket->ack_number = recv_header.seq_num + 1; //ACK = server.seq + 1
  socket->seq_number = my_seq + 1; 

  memset(&header, 0, sizeof(header));
  header.seq_num = socket->seq_number;
  header.ack_num = socket->ack_number;
  header.data_len = 0;
  header.window = MICROTCP_WIN_SIZE;
  header.control = (1 << 12); // Set ACK flag (Bit 12)

  header.checksum = crc32((const uint8_t *)&header, sizeof(header));

  if (sendto(socket->sd, &header, sizeof(header), 0, address, address_len) == -1) {
      return -1;
  }

  socket->state = ESTABLISHED;
  return 0;
}

int
microtcp_accept (microtcp_sock_t *socket, struct sockaddr *address,
                 socklen_t address_len)
{
  /* Your code here STEF*/
  //stefo na thimasai to ACK bit prepei na einai 1 sto packet p tha giriseis pisw gia to cconnection
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
  if(socket->state != ESTABLISHED) {
    return -1; //connection not established
  }

  microtcp_header_t header;
  memset(&header, 0, sizeof(header));
  header.data_len = length;
  header.window = socket->curr_win_size;
  header.seq_number = socket->seq_number;
  header.ack_number = socket->ack_number;
  
  header.control = (1 << 12); // Setting ACK=1, Bit 12
  //because this function is not called for handshaking packets, so ACK = 1
  header.checksum = crc32((const uint8_t *)&header, sizeof(header));

  //NA TO SINEXISW
  /* Your code here */
}

ssize_t
microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags)
{
  /* Your code here */
}
