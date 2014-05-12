// File: rudp.c    
// Created May 8, 2014
// Michael Baptist - mbaptist@ucsc.edu

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

// comment this out to turn on debug prints.
//#define NDEBUG NDEBUG

#include "rudp.h"
#include "utils.h"

#define SUCCESS    0
#define FAILURE    1

#define TRUE  1
#define FALSE 0

unsigned char *serialize_int(unsigned char *buffer, unsigned int val) {
    unsigned int size = sizeof(unsigned int);
    for (unsigned int i = 0; i < size; ++i) {
        buffer[i] = val >> (8*(size-i-1));
    }
    return buffer + size;
}

unsigned char *serialize_data(unsigned char *buffer, char buf[], int len) {
   for (int i = 0; i < len; ++i) {
       buffer[i] = (unsigned char)buf[i];
   }   
   return buffer + len;
}

unsigned char *serialize_packet(mftp_packet packet, unsigned char buffer[]) {
    buffer = serialize_int(buffer, packet.seq);
    buffer = serialize_data(buffer, packet.data, 1024);
    buffer = serialize_int(buffer, packet.flag);
    return buffer;
}

unsigned char *deserialize_int(unsigned char *buffer, unsigned int *val) {
    unsigned int size = sizeof(unsigned int);
    *val = 0;
    for (unsigned int i = 0; i < size; ++i) {
        unsigned int x = (unsigned int)buffer[i];
        *val +=(x << (8*(size-i-1)));
    }
    return buffer + size;
}

unsigned char *deserialize_data(unsigned char *buffer, char buf[], int len) {
   for (int i = 0; i < len; ++i) {
       buf[i] = (char)buffer[i];
   }   
   return buffer + len;
}


mftp_packet deserialize_packet(unsigned char buffer[]) {
    mftp_packet recv;
    buffer = deserialize_int(buffer, &recv.seq); 
    buffer = deserialize_data(buffer, recv.data, 1024);
    buffer = deserialize_int(buffer, &recv.flag);
    return recv;
}


void send_error(int seq, int clisock, const struct sockaddr_in client, int clen) {
   // send error.
   mftp_packet error;
   error.seq = seq;
   error.flag = ERROR;
   sprintf(error.data," ");
   int wc = send_dgram(clisock, &client, clen, error);
   if (wc == FALSE) {
      fprintf(stderr, "Error: sendto() error.\n");
   } else {
      DEBUGF("Write Success Error. Something went wrong..\n");
   }
}

void send_ack(int seq, int clisock, const struct sockaddr_in client, int clen) {
   // send ack.
   mftp_packet ack;
   ack.seq = seq;
   ack.flag = ACK;
   sprintf(ack.data," ");
   int wc = send_dgram(clisock, &client, clen, ack);
   if (wc == FALSE) {
      fprintf(stderr, "Error: ack sendto() error %d.\n", wc);
   } else {
      DEBUGF("Write Success (ack).\n");
   }
}

// returns 1 if success 0 if fail.
int send_dgram(int socket, const struct sockaddr_in *cli, int dlen, const mftp_packet data) {
    unsigned char buffer[2048], *ptr;
    bzero(buffer, sizeof(buffer));
    ptr = serialize_packet(data, buffer);
    int x = sendto(socket, buffer, (ptr - buffer), 0, (sockaddr*)cli, dlen);
    if (x != ptr - buffer) {
        fprintf(stderr, "%s", strerror(errno));
    }
    return x == (ptr - buffer);
}

mftp_packet parse_dgram(unsigned char buffer[]) {
    mftp_packet p = deserialize_packet(buffer);
    return p;
}
