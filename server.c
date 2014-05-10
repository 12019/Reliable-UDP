// File: server.c    
// Created April 7, 2014
// Michael Baptist - mbaptist@ucsc.edu

/*******
NAME
     server -- returns a formatted time to a requesting client

SYNOPSIS
     server [Port]

DESCRIPTION  
     This program accepts the client port number as it's arguments,
     and while running, if contacted by a client, returns a chunk 
     of a file to the user.

OPERANDS
     The only operand is a valid unused port number. If no port 
     number is input to the program, the program will exit with
     an error.

EXIT STATUS

     0    No errors were detected.
     1    One or more errors were detected and error messages were
          printed.
 errno    If a man page call detects an error, perror prints a sys
          message and exits with return value of errno.

******/


#include <stdio.h>       // standard I/O
#include <stdlib.h>      // standard library
#include <string.h>      // conatains memset
#include <sys/types.h>   // prerequisite typedefs 
#include <errno.h>       // error numbers for sys calls
#include <sys/socket.h>  // struct sockaddr; system prototypes and consts
#include <netdb.h>       // network info lookup prototypes and stuctures 
#include <netinet/in.h>  // struct sockaddr_in; byte ordering macros
#include <arpa/inet.h>   // utility function prototypes
#include <unistd.h>      // uni-standard lib.h
#include <sys/select.h>  // used for select io multiplexing.
#include <time.h>        // for the random numbers
#include <sys/ioctl.h>   // allows nonblocking sockets.
#include <pthread.h>     // allows for threaded server.

// comment this out to turn on debug print statements.
//#define NDEBUG NDEBUG

#include "utils.h"
#include "rudp.h"

#define SUCCESS   0
#define FAILURE   1

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

static uint8_t exit_status = SUCCESS;
static uint8_t threadcount = 0;
static int listening_port = 0;

// handle a client request gets the data from the server and sends it.
void *handle_client_request(void *clisock);

// closes a client socket and removes it from an fd_set
void close_client(int clisock, fd_set *master);


struct client_ip_port {
    unsigned long ip;
    unsigned short port;
};

unsigned char *serialize_datastruct(struct client_ip_port p, unsigned char buffer[]);

struct client_ip_port deserialize_datastruct(unsigned char buffer[]);

int main(int argc, char **argv) {
  //initial error checking
  if (argc != 2) {
    fprintf(stderr, "Error: Include Listening Port Number.\n");
    fprintf(stderr, "Usage: %s [PORT]\n", argv[0]);
    exit_status = FAILURE;
    return FAILURE;
  }
  char *endptr = NULL;
  int portnum = (int)strtol(argv[1], &endptr, 10);
  if (0 == portnum || *endptr != '\0') {
     // error handling not valid port number
     fprintf(stderr, "Error: Invalid Port Number: %s\n", argv[1]);
     exit_status = FAILURE;
     return FAILURE;
  } else {
     DEBUGF("Server creates connections on port number: %d\n", portnum);
     listening_port = portnum;
  }

  //create socket for the server 
  int serv_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (serv_socket < 0) {
     perror("ERROR: SOCKET CORRUPT ");
     return errno;
  } else {
     DEBUGF("Server Socket: %d\n", serv_socket);
  }

  sockaddr_in my_serv;
  my_serv.sin_family = AF_INET;
  my_serv.sin_port = htons(portnum);
  my_serv.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(my_serv.sin_zero, '\0', sizeof(my_serv.sin_zero));
  sockaddr_in *my_serv_ref = &my_serv;
  int er_chk = bind(serv_socket, (sockaddr*)my_serv_ref, sizeof(my_serv));
  if (er_chk < 0) {
     perror("Error: Socket to Address bind failure ");
     return errno;
  } else {
     DEBUGF("Bind Success.\n");
  }

  /* 
  // test serialization
  mftp_packet packet;
  packet.seq = 123192;
  packet.flag = 95823;
  char *buf = "DATA TO SEND TEST";
  unsigned char buffer[2048];
  memcpy (packet.data, buf, 19);
  DEBUGF("%s, %d, %d\n", packet.data, packet.flag, packet.seq);
  bzero(buffer, sizeof(buffer));
  serialize_packet(packet, buffer);
  DEBUGF("%s\n", buffer);
  mftp_packet ret = deserialize_packet(buffer);
  DEBUGF("%s, %d, %d\n", ret.data, ret.flag, ret.seq);
  */

  while (1) {
      DEBUGF("Main thread waiting for connections on listening port.\n");
      sockaddr_in client;
      sockaddr_in *cli_ref = &client;
      uint clen = (uint)sizeof(client);
      char buffer[4];
      bzero(buffer, sizeof(buffer));
      int rc = recvfrom(serv_socket, buffer, sizeof(buffer),
                        0, (sockaddr*)cli_ref, &clen);
      if (rc == -1) {
          fprintf(stderr, "Error: recvfrom() failed.\n");
      } else {
          DEBUGF("recieved from :%s, on port: %hu\n", inet_ntoa(client.sin_addr), client.sin_port);
      }
      
      // set up thread for new connection
      pthread_t thread_ID; 
      struct client_ip_port cliinfo;
      cliinfo.ip = client.sin_addr.s_addr;
      cliinfo.port = client.sin_port;
      unsigned char infobuffer[(sizeof(cliinfo))];
      bzero(infobuffer, sizeof(infobuffer));
      serialize_datastruct(cliinfo, infobuffer);

      // Create the thread, passing &value for the argument.
      threadcount++;
      int i = pthread_create(&thread_ID, NULL, handle_client_request, 
                                                             (void *)infobuffer);
      if (i == EAGAIN) {
          fprintf(stderr, "Error: Insufficient resources to create another \
                           thread,  or  a  system imposed  limit  on  \
                           the  number of threads was encountered.  \
                           The latter case  may  occur  in  two  ways:\
                           the  RLIMIT_NPROC  soft resource  limit  (set \
                           via setrlimit(2)), which limits the number of \
                           process for a real user ID, was reached; \
                           or the kernel's system-wide   limit   on  \
                           the  number  of  threads,  /proc/sys/kernel\
                           /threads-max, was reached.\n");
      } else if (i == EINVAL) {
          fprintf(stderr, "Error: Invalid settings in attr.\n");
      } else if (i == EPERM) {
          fprintf(stderr, "Error: No permission to set the scheduling\
                           policy and parameters specified in attr.\n");
      }
  }

  // program never gets here, but if there was an exit 
  // server this would close the server. 
  close(serv_socket);

  return exit_status;
}


void *handle_client_request(void *c) {
    DEBUGF("New pthread created to handle client.\n");

    struct client_ip_port cliinfo = deserialize_datastruct((unsigned char *)c);

    // get client port and address from struct c
    sockaddr_in client;
    uint clen = (uint)sizeof(client);
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = (unsigned long)cliinfo.ip;
    client.sin_port = (unsigned short)cliinfo.port; 
    DEBUGF("client address: %s, port: %hu\n", inet_ntoa(client.sin_addr), client.sin_port);
 
   //create new socket for the server to send back to this client. 
   int clisock = socket(AF_INET, SOCK_DGRAM, 0);
   if (clisock < 0) {
      perror("ERROR: SOCKET CORRUPT ");
      return (void*)errno;
   } else {
      DEBUGF("Server Socket: %d\n", clisock);
   }
 
   // bind the new socket to a different port than the listening port.
   sockaddr_in serv_sock;
   serv_sock.sin_family = AF_INET;
   serv_sock.sin_port = htons(client.sin_port);
   serv_sock.sin_addr.s_addr = htonl(INADDR_ANY);
   memset(serv_sock.sin_zero, '\0', sizeof(serv_sock.sin_zero));
   sockaddr_in *serv_sock_ref = &serv_sock;
   int er_chk = bind(clisock, (sockaddr*)serv_sock_ref, sizeof(serv_sock));
   if (er_chk < 0) {
      perror("Error: Socket to Address bind failure ");
      return (void*)errno;
   } else {
      DEBUGF("Bind Success on port:%hu.\n", serv_sock.sin_port);
   }

   // create MAIN timeout for if the connection goes dead for a while then
   // exit the thread. here i can use alarm() and signal(SIGARLRM, handler)
   // or the timer method from the forums.

   // create fd_set to use for the timeout. 
   fd_set master;            // master list of file descriptors.
   FD_ZERO(&master);
   FD_SET(clisock, &master); // add listening socket to master list.

   // send first packet to client on new port so it knows to start 
   // sending here.
   send_ack(1, clisock, client, clen);
 
   // loop forever until the exit or done message is given.
   // if the timeval tv expires in select a retranmission should occur.
   int last_packet = ACK;
   int last_packet_seq = 1;
   mftp_packet last_p;
   char state = 1;       
   FILE *fileserv = NULL;
   char filename[256];
   int chunksize = 0;
   int bytessent = 0;
   int offset = 0;
   int breakloop = 0;
   int connection_timeouts = 0;
   while (1) {

       DEBUGF("Posix thread waiting on select().\n");
       struct timeval tv = {0,0};
       tv.tv_sec = 5;
       fd_set read_fds = master;

       if (breakloop) break; // break from select if done transmitting.

       if (select(clisock + 1, &read_fds, NULL, NULL, &tv) < 0) {
          fprintf(stderr, "Error: select() failed.\n");
          if (errno == EBADF) {
             fprintf(stderr, "Error: select() failed due to bad descriptor.\n");
          }
          exit_status = FAILURE;
       }
       DEBUGF("select() has returned.\n");

       if (FD_ISSET(clisock, &read_fds)) {
          // process client response.
          unsigned char buffer[1100];
          bzero(buffer, sizeof(buffer));
          int result = recvfrom(clisock, buffer, sizeof(buffer),
                        0, (sockaddr*)&client, &clen);
          if (result == -1) {
              // handle error
              perror("Error: recvfrom() failed. ");
              return (void*)errno;
          } else {
              mftp_packet p = parse_dgram(buffer);
              DEBUGF("data = %s, flag = %d, seq = %d, state = %d.\n", p.data, p.flag, p.seq, state);
              // process packet
              switch (state) {
                case 1: // send ack for if valid file. otherwise error
                {
                    // search for file in directory.
                    fileserv = retrieve_file(p.data, "r");
                    // if no such file then break out and serv new client.
                    if (fileserv == NULL) {
                       send_error(p.seq, clisock, client, clen);
                       close_client(clisock, &master);
                       return NULL;
                    } else {
                       DEBUGF("File: %s requested.\n", p.data);
                       sprintf(filename, "%s", p.data);
                       send_ack(p.seq, clisock, client, clen);
                       last_packet = ACK;
                       last_packet_seq = p.seq;
                       state = 2;
                    }
                    break;
                }
                case 2: // parse get connect num and send ack.
                {
                    // parse connect num
                    DEBUGF("Getting number of connections.\n");
                    char *endptr = NULL;
                    int cnum = (int)strtol(p.data, &endptr, 10);
                    if (*endptr != '\0') {
                       fprintf(stderr, "Error: Invalid chunksize value: %s.\n", p.data);
                       send_error(1, clisock, client, clen);
                       close_client(clisock, &master);
                       return NULL;
                    } else {
                       int filesize = get_file_size(fileserv);
                       chunksize = filesize / cnum;
                       DEBUGF("%d connections => chunksize = %d\n", cnum, chunksize);
                       send_ack(p.seq, clisock, client, clen);
                       last_packet = ACK;
                       last_packet_seq = p.seq;
                       state = 3;
                    }
                    break;
                }
                case 3: // send ack for thet starting point of the file
                {
                    // parse offset value.
                    char *endptr = NULL;
                    offset = (int)strtol(p.data, &endptr, 10);
                    if (*endptr != '\0') {
                       fprintf(stderr, "Error: Invalid file offset value: %s.\n", p.data);
                       send_error(1, clisock, client, clen);
                       close_client(clisock, &master);
                       return NULL;
                    } else {
                       DEBUGF("Filename: %s. Chunksize: %d. Offset: %d.\n", filename, chunksize, offset);
                       send_ack(p.seq, clisock, client, clen);
                       last_packet = ACK;
                       last_packet_seq = p.seq;
                       state = 4;
                    }
                    break;
                }
                case 4: // receive ack and send next data.
                {
                    if (bytessent <= chunksize) {
                        mftp_packet data = get_file_chunk(chunksize*offset + bytessent, fileserv, p.seq);
                        int wc = send_dgram(clisock, &client, clen, data);
                        if (wc == 0) {
                            fprintf(stderr, "Error: sendto()) error.\n");
                        } else {
                            bytessent += 1023;
                            DEBUGF("Write Success (data) in state 4, %d.\n", bytessent);
                            last_p = data;
                            last_packet_seq = p.seq;
                        }
                    } else {
                        state = 5;
                    }
                    break;
                }
                case 5: // done with transmission
                    send_ack(p.seq, clisock, client, clen);
                    last_packet = ACK;
                    last_packet_seq = p.seq;
                    breakloop = 1;
                    break;
                default: // no default case.
                    break;
              }
          }
       } else {
          // handle timeout
          // retransmit last packet.
          connection_timeouts++;
          if (connection_timeouts > 5) {
              breakloop = 1;
          }
          if (last_packet == ACK) {
             // retransmit ack
             send_ack(last_packet_seq, clisock, client, clen);
          } else {
              int wc = send_dgram(clisock, &client, clen, last_p);
              if (wc == 0) {
                  fprintf(stderr, "Error: sendto()) error.\n");
              } else {
                  DEBUGF("Write Success (data) resend*, %d.\n", bytessent);
              }
          }
       }
       if (breakloop) {
           break;
       }
   }
   close_client(clisock, &master);  
   return NULL;
}

void close_client(int clisock, fd_set *master) {
   DEBUGF("closing client socket: %d.\n", clisock);
   close(clisock);
   FD_CLR(clisock, master);
}



unsigned char *serialize_datastruct(struct client_ip_port p, unsigned char buffer[]) {
    buffer = serialize_int(buffer, (int)p.ip);
    buffer = serialize_int(buffer, (int)p.port);
    return buffer;
}

struct client_ip_port deserialize_datastruct(unsigned char buffer[]) {
    unsigned int ip = 0;
    unsigned int port = 0;
    buffer = deserialize_int(buffer, &ip);
    buffer = deserialize_int(buffer, &port);
    struct client_ip_port p;
    p.ip = (unsigned long)ip;
    p.port = (unsigned short)port;
    return p;
}
