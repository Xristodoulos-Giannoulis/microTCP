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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

microtcp_sock_t
microtcp_socket (int domain, int type, int protocol)
{
  microtcp_sock_t this_sock;
  int sock;
  if ((sock = socket ( domain , type , protocol )) == -1) {
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

}

int
microtcp_bind (microtcp_sock_t *socket, const struct sockaddr *address,
               socklen_t address_len)
{
  if ( bind ( socket->sd , address , address_len ) == -1 ) {
    perror ( " BINDING FAILED " );
    exit ( EXIT_FAILURE );
  }
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

      if (bytes < sizeof(microtcp_header_t)) continue;
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
  microtcp_header_t headerReceived;
  microtcp_header_t headerSent;
  microtcp_sock_t connection_socket;
  uint32_t received_checksum;
  ssize_t bytesReceived;
  
  while (1) {                           // wait for incoming SYN
    bytesReceived = recvfrom(socket->sd, socket->recvbuf, MICROTCP_RECVBUF_LEN, 0,
                             address, &address_len);
    if (bytesReceived > 0) {
      memcpy(&headerReceived, socket->recvbuf, sizeof(microtcp_header_t));
      if (!(headerReceived.control & MICROTCP_SYN)) {       // error when the packet sent is not a SYN
        perror("CONTROL ERROR");
        continue;
      }
      microtcp_header_t *hdr = (microtcp_header_t *)socket->recvbuf;
      received_checksum = headerReceived.checksum;
      headerReceived.checksum = 0;                         // set checksum  to 0 before calculating it
      if (crc32((const uint8_t *)&headerReceived, sizeof(headerReceived)) != received_checksum) {
        perror("CHECKSUM ERROR");
        continue;
      }
      if (bytesReceived < sizeof(microtcp_header_t)) {      // error when packet size is smaller than header size
        perror("SIZE ERROR");
        continue;
      }

      break;                            // break when first packet without errors is received
    }
    else if (bytesReceived == -1) {     // error when recvfrom returns -1
      perror("RECEIVE ERROR");
      exit(EXIT_FAILURE);
    }
  }

  connection_socket = microtcp_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // create new server socket for the connection
  connection_socket.state = HANDSHAKE;
  connection_socket.seq_number = rand();                 // make the state up to date
  connection_socket.ack_number = headerReceived.seq_number + 1;

  memset(&headerSent, 0, sizeof(headerSent));
  headerSent.seq_number = connection_socket.seq_number;
  headerSent.ack_number = headerReceived.seq_number + 1;
  headerSent.data_len = 0;
  headerSent.window = MICROTCP_WIN_SIZE;
  headerSent.control = MICROTCP_SYN | MICROTCP_ACK;                        // set SYN and ACK flags
  headerSent.future_use0 = 0;                                              // must set checksum and unused fields to 0
  headerSent.future_use1 = 0;
  headerSent.future_use2 = 0; 
  headerSent.checksum = 0;
  headerSent.checksum = crc32((const uint8_t *)&headerSent, sizeof(headerSent));
  if (sendto(connection_socket.sd, &headerSent, sizeof(headerSent), 0, address, address_len) == -1) {
    perror("SEND ERROR");
    exit(EXIT_FAILURE);
  }
  return connection_socket.sd;
}

int
microtcp_shutdown (microtcp_sock_t *socket, int how)
{

  microtcp_header_t headerReceived;
  microtcp_header_t headerSent;
  uint32_t received_checksum;

  if (socket->state == ESTABLISHED) {                 // check for the correct state
    memset(&headerSent, 0, sizeof(headerSent));
    headerSent.seq_number = rand();
    headerSent.control    = MICROTCP_FIN | MICROTCP_ACK;
    headerSent.window     = socket->curr_win_size;

    headerSent.checksum = 0;
    headerSent.checksum = crc32((const uint8_t *)&headerSent, sizeof(headerSent));

    sendto(socket->sd, &headerSent, sizeof(headerSent), 0, peer, peer_len);           // send ACK for FIN
    socket->ack_number = headerReceived->seq_number + 1;                              // update the state of the socket
    socket->state = CLOSING_BY_PEER;
  }
  microtcp_server_finish(peer, &headerReceived, socket, socket_len);                    
  micro_tcp_client_finish(socket, &headerReceived, peer, peer_len);
  microtcp_server_finish(peer, &headerReceived, socket, socket_len);
  
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

  /* Your code here */
}

ssize_t
microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags)
{
  /* Your code here */
}

void micro_tcp_client_finish(microtcp_sock_t *socket,
                            microtcp_header_t *headerReceived,
                            struct sockaddr *peer,
                            socklen_t peer_len)
{
  microtcp_header_t headerSent;
  uint32_t received_checksum;
  ssize_t bytesReceived;
  
  if (headerReceived != NULL) {

    if ((headerReceived->control & MICROTCP_FIN) &&
      (headerReceived->control & MICROTCP_ACK)) {         // if client received FIN + ACK

      if (socket->state == ESTABLISHED) {                 // check for the correct state

        memset(&headerSent, 0, sizeof(headerSent));
        headerSent.seq_number = socket->seq_number;
        headerSent.ack_number = headerReceived->seq_number + 1;
        headerSent.control    = MICROTCP_ACK;
        headerSent.window     = socket->curr_win_size;

        headerSent.checksum = 0;
        headerSent.checksum = crc32((const uint8_t *)&headerSent, sizeof(headerSent));

        sendto(socket->sd, &headerSent, sizeof(headerSent), 0, peer, peer_len);           // send ACK for FIN

        socket->ack_number = headerReceived->seq_number + 1;                              // update the state of the socket
        socket->state = CLOSED;
        return;
      }
    }
    else if (headerReceived->control & MICROTCP_ACK) {    // if server received ACK

      if (socket->state == ESTABLISHED) {
        socket->state = CLOSING_BY_HOST;
        return;
      }
    }
  }
}

void microtcp_server_finish(microtcp_sock_t *socket,
                            microtcp_header_t *headerReceived,
                            struct sockaddr *peer,
                            socklen_t peer_len)
{
  microtcp_header_t headerSent;
  microtcp_header_t headerSent2;
  uint32_t received_checksum;
  ssize_t bytesReceived;
  
  if (headerReceived != NULL) {

    if ((headerReceived->control & MICROTCP_FIN) &&
      (headerReceived->control & MICROTCP_ACK)) {         // if server received FIN + ACK

      if (socket->state == ESTABLISHED) {                 // check for the correct state

        memset(&headerSent, 0, sizeof(headerSent));
        headerSent.seq_number = socket->seq_number;
        headerSent.ack_number = headerReceived->seq_number + 1;
        headerSent.control    = MICROTCP_ACK;
        headerSent.window     = socket->curr_win_size;

        headerSent.checksum = 0;
        headerSent.checksum = crc32((const uint8_t *)&headerSent, sizeof(headerSent));

        sendto(socket->sd, &headerSent, sizeof(headerSent), 0, peer, peer_len);           // send ACK for FIN

        socket->ack_number = headerReceived->seq_number + 1;                              // update the state of the socket
        socket->state = CLOSING_BY_PEER;
      }
    }
    else if (headerReceived->control & MICROTCP_ACK) {    // if server received ACK

      if (socket->state == CLOSING_BY_PEER) {
        socket->state = CLOSED;
        return;
      }
    }
  }
  if (socket->state == CLOSING_BY_PEER) {             // send FIN + ACK
    memset(&headerSent2, 0, sizeof(headerSent2));
    headerSent2.seq_number = socket->seq_number;
    headerSent2.ack_number = socket->ack_number;
    headerSent2.control    = MICROTCP_FIN | MICROTCP_ACK;
    headerSent2.window     = socket->curr_win_size;

    headerSent2.checksum = 0;
    headerSent2.checksum = crc32((const uint8_t *)&headerSent2, sizeof(headerSent2));
    
    sendto(socket->sd, &headerSent2, sizeof(headerSent2), 0, peer, peer_len);
    socket->seq_number = rand();
    return;
  }
}