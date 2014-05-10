// File: client.c    
// Created April 7, 2014
// Michael Baptist - mbaptist@ucsc.edu

/*******
NAME
     client -- contacts a server to obtain a chunk of a file from the server 
               using pthreadds. Collects all chunks then it puts them together 
               to form a whole file.

SYNOPSIS
     client <filename> <number of connections>

DESCRIPTION
     This program contacts a server to obtain a chunk of a file from the server 
     using pthreadds. Collects all chunks then it puts them together 
     to form a whole file.

OPERANDS
     The two operands are first an filename to be retreived, and second a 
     number of threads that it wants to use to collect the file using. Really 
     then number of server connections it wants to make. 

EXIT STATUS

     0    No errors were detected.
     1    One or more errors were detected and error messages were
          printed.
 errno    If a man page call detects an error, perror prints a sys
          message and exits with return value of errno.

******/


#include <arpa/inet.h>   // utility function prototypes
#include <errno.h>       // error numbers for sys calls
#include <netdb.h>       // network info lookup prototypes and stuctures 
#include <netinet/in.h>  // struct sockaddr_in; byte ordering macros
#include <stdlib.h>      // standard library
#include <stdio.h>       // standard I/O
#include <sys/types.h>   // prerequisite typedefs 
#include <sys/socket.h>  // struct sockaddr; system prototypes and consts
#include <time.h>        // time() func
#include <unistd.h>      // close(2)
#include <string.h>      // string lib
#include <pthread.h>     // pthread lib

// comment out for no debugging prints statements
//#define NDEBUG NDEBUG

// utilities library for this program.
#include "utils.h"
#include "rudp.h"

#define SUCCESS    0
#define FAILURE    1
#define FALSE      0

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

static uint8_t exit_status = SUCCESS;

// checks for "ERROR" in buffers which is an app layer error from teh server.
void check_error(char *x, char *y);

void *thread_get_chunk(void *arg);

struct threadargs {
    unsigned int cnum; // connect number
    char address[128];
    unsigned int port;
    char filename[256]; // filename to get
    unsigned int validipnum;
};

unsigned char *serialize_threadargs(struct threadargs, unsigned char buffer[]);
struct threadargs deserialize_threadargs(unsigned char buffer[]);

int main(int argc, char **argv) {
  // Usage check
  // Only allow 3 or 4 arguments.
  if (argc != 3) {
    fprintf(stderr, "Usage: %s [filename> <num-connections>\n", argv[0]);
    exit_status = FAILURE;
    return exit_status;
  }

   char *filename = NULL;
   int connectnum = 0;
   
   opterr = FALSE;
   for (;;) {
      int option = getopt (argc, argv, "");
      if (option == EOF) {
         char *endptr = NULL;
         if (3 == argc) {
             filename = argv[1];
             connectnum = (uint16_t)strtol(argv[2], &endptr, 10);
         }
         if (*endptr != '\0') {
            fprintf(stderr, "Error: Invalid number of connections: %s.\n", argv[2]);
            return FAILURE;
         }
         DEBUGF("Filename: %s. Connections: %d.\n", filename, connectnum);
         break;
      }
      switch (option) {
         default : fprintf (stderr, "Error: -%c: invalid option\n", optopt);
                   fprintf(stderr, "Usage: %s [filename> <num-connections>\n", argv[0]);
                   exit_status = FAILURE;
                   return exit_status;
      };
   };


   pthread_t threadID[connectnum];
   void *threadexit[connectnum];
  
  //Load up structs with the server information
  // find information from the file server-info.txt

  //FILE *serverlist = fopen("server-info.txt", "r");
  FILE *serverlist = retrieve_file("server-info.txt", "r");
  if (serverlist == NULL) {
     perror(" file server-info.txt not found");
     return(errno);
  }
  char buf[64];
  bzero(buf, sizeof(buf));
  int validipnum = 0;
  while(fgets(buf, sizeof(buf), serverlist) != NULL) {
    char *nlpos = strchr(buf, '\n');
    if (nlpos != NULL) {
       *nlpos = '\0';
    } 

    // get server ip and port from server list.
    char *tok = buf;
    int i = 0;
    char *servaddr = NULL;
    int port = 0;
    int line_status = SUCCESS;
    while ((tok = strtok(tok, " ")) != NULL) {
        // parse argument string
        sockaddr_in sockinfo;
        if (0 == i) {
           servaddr = tok;
           if (inet_aton(servaddr, &sockinfo.sin_addr) == 0) {
              fprintf(stderr, "Error: invalid IP address: %s in server-info.txt. Disreguarding line.\n", servaddr);
              line_status = FAILURE;
              break;
           }
           ++i; 
        } else if (1 == i) {
             char *endptr = NULL;
             port = (uint16_t)strtol(tok, &endptr, 10);
             if (*endptr != '\0') {
                fprintf(stderr, "Error: Invalid port number: %s in server-info.txt. Disreguarding line.\n", tok);
                line_status = FAILURE;
                break;
             }
             sockinfo.sin_port = htons(port);
        } else {
           fprintf(stderr, "Error: invalid line: %s. Disreguarding line.\n", buf);
           line_status = FAILURE;
           break;
        }
        tok = NULL;
    }

    // create pthread for each server in the list if line is valid.
    if (line_status == SUCCESS) {
        DEBUGF("ip addr: %s port: %d\n", servaddr, port);

        struct threadargs p;
        p.cnum = connectnum;
        memcpy(p.address, servaddr, 64);
        p.port = port;
        memcpy(p.filename, filename, 256);
        p.validipnum = validipnum;
        unsigned char buffer[2048];
        bzero(buffer, sizeof(buffer));
        serialize_threadargs(p, buffer);
        pthread_create(&threadID[validipnum], NULL, thread_get_chunk, (void*)buffer);
        validipnum++;
        if (validipnum == connectnum) break;
    }
  }

  
  void *result = NULL;
  for (int i = 0; i < validipnum; ++i) {
    result = threadexit[i];
    pthread_join(threadID[i], &result);
    DEBUGF("Thread returned. valid:%d\n", validipnum);
    threadexit[i] = result;
  }

  /*
  for (int i = 0; i < validipnum; ++i) {
    FILE *newfile = fopen(filename, "w+");
    if (newfile == NULL) {
       fprintf(stderr, "Error: Creation of file: %s failed.\n", filename);
       return FAILURE;
    } else {
       DEBUGF("File opened.\n");
    }
    char *data = (char *)threadexit[i];
    int wc = write(fileno(newfile), data, cs);
    if (wc < 0) {
       fprintf(stderr, "Error: write(2) error when write to file: %s.\n", filename);
       return FAILURE;
    } else {
       DEBUGF("Write Success: %s.\n", filename);
       free(result);
    }
  }
  */
  
  return SUCCESS;
}


void *thread_get_chunk(void *arg) { 
   // parse argument string
   sockaddr_in sockinfo;
   sockinfo.sin_family = AF_INET; 
   struct threadargs targs = deserialize_threadargs((unsigned char *)arg);
   DEBUGF("thread recieved: %d, %s, %d, %s, %d\n", targs.cnum, targs.address, targs.port, targs.filename, targs.validipnum);

   // Open the client socket and check that it is valid.
   int clisock = socket(AF_INET, SOCK_DGRAM,0);
   if (clisock < 0) {
       perror("ERROR: SOCKET CORRUPT ");
       return (void*)errno;
   } else {
       DEBUGF("Client Socket: %d\n", clisock);
   }

   // set up server information
   int seqnum = 1;
   sockinfo.sin_port = htons(targs.port);
   inet_aton(targs.address, &sockinfo.sin_addr);

   // send empty packet to server
   DEBUGF("Sending ack to server to start data transfer.\n");
   send_ack(seqnum++, clisock, sockinfo, sizeof(sockinfo));

   // create MAIN timeout for if the connection goes dead for a while then
   // exit the thread. here i can use alarm() and signal(SIGARLRM, handler)
   // or the timer method from the forums.

   // create fd_set to use for the timeout. 
   fd_set master;            // master list of file descriptors.
   FD_ZERO(&master);
   FD_SET(clisock, &master); // add listening socket to master list.

   // loop forever until the exit or done message is given.
   // if the timeval tv expires in select a retranmission should occur.
   int last_packet = ACK;
   int last_packet_seq = seqnum - 1;
   int file_seq_num = 0;
   mftp_packet last_p;
   char state = 1;
   int breakloop = 0;
   sockaddr_in servinfo;
   uint slen = (uint)sizeof(servinfo);
   int connection_timeouts = 0;

   // loop until entire chunk of file has been recieved. 
   while (1) {

       DEBUGF("Posix thread waiting on select().\n");
       struct timeval tv = {0,0};
       tv.tv_sec = 5;
       fd_set read_fds = master;

       if (state == 6) break;// break from while if we reached last stage

       if (select(clisock + 1, &read_fds, NULL, NULL, &tv) < 0) {
          fprintf(stderr, "Error: select() failed.\n");
          if (errno == EBADF) {
             fprintf(stderr, "Error: select() failed due to bad descriptor.\n");
          }
          exit_status = FAILURE;
       }
       DEBUGF("select() has returned.\n");

       if (FD_ISSET(clisock, &read_fds)) {
          // process server response.
          unsigned char buffer[1100];
          bzero(buffer, sizeof(buffer));
          int result = recvfrom(clisock, buffer, sizeof(buffer),
                        0, (sockaddr*)&servinfo, &slen);
          if (result == -1) {
              // handle error
              perror("Error: recvfrom() failed. Exiting thread.");
              return (void*)errno;
          } else {
              mftp_packet sdata = parse_dgram(buffer);
              DEBUGF("data = %s, flag = %d, seq = %d.\n", sdata.data, sdata.flag, sdata.seq);

              // if server sends an error exit thread. 
              if (sdata.flag == ERROR) {
                  return NULL;
              }

              // process packet
              switch (state) {
                case 1: // send filename
                {
                    mftp_packet fileinfo;
                    fileinfo.flag = DATA;
                    fileinfo.seq = seqnum++;   
                    memcpy(fileinfo.data, targs.filename, 256);
                    last_p = fileinfo;
                    DEBUGF("File: %s requested. Sending to server.\n", fileinfo.data);
                    int wc = send_dgram(clisock, &servinfo, slen, fileinfo);
                    if (wc == FALSE) {
                       fprintf(stderr, "Error: sendto()) error.\n");
                       close(clisock);
                       return NULL;
                    }
                    last_packet = DATA;
                    last_packet_seq = fileinfo.seq;
                    state = 2;
                    break;
                }
                case 2: // send connect num to server.
                {
                    mftp_packet connect_num;
                    sprintf(connect_num.data, "%d", targs.cnum);
                    connect_num.flag = DATA;
                    connect_num.seq = seqnum++;
                    last_p = connect_num;
                    DEBUGF("Connect num being sent: %s.\n", connect_num.data);
                    int wc = send_dgram(clisock, &servinfo, slen, connect_num);
                    if (wc == FALSE) {
                       fprintf(stderr, "Error: sendto()) error.\n");
                       close(clisock);
                       return NULL;
                    }
                    last_packet = DATA;
                    last_packet_seq = connect_num.seq;
                    state = 3; 
                    break;
                }
                case 3: // send starting point of the file
                {
                    mftp_packet offset;
                    sprintf(offset.data, "%d", targs.validipnum);
                    offset.flag = DATA;
                    offset.seq = seqnum++;
                    last_p = offset;
                    DEBUGF("Connect num being sent: %s.\n", offset.data);
                    int wc = send_dgram(clisock, &servinfo, slen, offset);
                    if (wc == FALSE) {
                       fprintf(stderr, "Error: sendto()) error.\n");
                       close(clisock);
                       return NULL;
                    }
                    last_packet = DATA;
                    last_packet_seq = offset.seq;
                    state = 4; 
                    break;
                }
                case 4: 
                {
                    last_packet_seq = seqnum++;
                    file_seq_num = last_packet_seq;
                    send_ack(seqnum, clisock, servinfo, slen);
                    last_packet = ACK;
                    state = 5;
                    break;
                }
                case 5: // receive data send next ack.
                {
                    if (sdata.flag == DATA) {
                        char buffer[64];
                        sprintf(buffer, "thread%d", targs.validipnum);
                        FILE *newfile = NULL;
                        if (file_seq_num == last_packet_seq) {
                            newfile = fopen(buffer, "w+");
                        } else {
                            newfile = fopen(buffer, "a");
                        }
                        if (newfile == NULL) {
                           fprintf(stderr, "Error: Opening of file: %s failed.\n", buffer);
                           close(clisock);
                           return NULL;
                        } else {
                           DEBUGF("File %s opened.\n", buffer);
                        }
                        int wc = write(fileno(newfile), sdata.data, strlen(sdata.data));
                        if (wc < 0) {
                           fprintf(stderr, "Error: write(2) error when write to file: %s.\n", buffer);
                           close(clisock);
                           return NULL;
                        }
                        last_packet_seq = seqnum++;
                        send_ack(seqnum, clisock, servinfo, slen);
                    } else {
                        state = 6;
                    }
                    break;
                }
                case 6: // done with transmission
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
             send_ack(seqnum, clisock, servinfo, slen);
          } else {
              int wc = send_dgram(clisock, &servinfo, slen, last_p);
              if (wc == 0) {
                  fprintf(stderr, "Error: sendto()) error.\n");
              } else {
                  DEBUGF("Resending last data. Write Success (data), %d.\n");
              }
          }
       }
       if (breakloop) {
           break;
       }
   }
   close(clisock);  
   return NULL;
}

void check_error(char *x, char *y) {
   if (strcmp(x, y) == 0) {
      fprintf(stderr, "Error: server send an error. Exiting.\n");
      exit(FAILURE);
   }
}


unsigned char *serialize_threadargs(struct threadargs p, unsigned char buffer[]) {
    buffer = serialize_int(buffer, p.cnum);
    buffer = serialize_data(buffer, p.address, 128);
    buffer = serialize_int(buffer, p.port);
    buffer = serialize_data(buffer, p.filename, 256);
    buffer = serialize_int(buffer, p.validipnum);
    return buffer;
}

struct threadargs deserialize_threadargs(unsigned char buffer[]) {
    struct threadargs p;
    buffer = deserialize_int(buffer, &p.cnum);
    buffer = deserialize_data(buffer, p.address, 128);
    buffer = deserialize_int(buffer, &p.port);
    buffer = deserialize_data(buffer, p.filename, 256);
    buffer = deserialize_int(buffer, &p.validipnum);
    return p;
}
