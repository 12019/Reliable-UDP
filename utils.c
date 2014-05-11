// File: utils.c    
// Created April 7, 2014
// Michael Baptist - mbaptist@ucsc.edu

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>

// comment this out to turn on debug prints.
//#define NDEBUG NDEBUG

#include "utils.h"
#include "rudp.h"

#define SUCCESS    0
#define FAILURE    1


void check_socket(int fd) {
   if (fd < 0) {
      perror("Error: Socket Corrupt ");
      exit(errno);
   } else {
      DEBUGF("Client Socket: %d In Use\n", fd);
   }
}

void check_connection(int x) {
  if (x < 0) {
    perror("Error: Connection Failed ");
    exit(errno);
  } else {
    DEBUGF("Connection Established\n");
    DEBUGF("Server Socket %d Connected\n", x);
  }
}

FILE *retrieve_file(const char *restrict filename, const char* restrict mode) {
   DIR *d = NULL;
   struct dirent *dir = NULL;
   d = opendir(".");
   if (d) {
      while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, filename) == 0) {
             FILE *file = fopen(filename, mode);
             closedir(d);
             return file; 
          }
      }
      fprintf(stderr, "Error: no file matching %s exists in current directory.\n", filename);
      return NULL;
   }
   fprintf(stderr, "Error: no directory matching . exists.\n");
   return NULL;
}

int get_file_size(FILE *restrict file) {
   fseek(file, 0, SEEK_END); // seek to end of file
   int size = (int)ftell(file); // get current file pointer
   fseek(file, 0, SEEK_SET); // seek back to beginning of file
   return size;
}

mftp_packet get_file_chunk(int f_offset, FILE *restrict stream, int seq) {
    char buffer[1024]; // buffer to hold file chunk
    bzero(buffer, sizeof(buffer));
    int ls = lseek(fileno(stream), f_offset, SEEK_SET);
    if (ls == -1) {
       fprintf(stderr, "Error: seeking to requested chunk location failed. File buffer pointer at unknown location.\n");
    }
    int numbytes = 0;
    while (numbytes != 1023) {
       int x = read(fileno(stream), buffer, sizeof(buffer) - 1);
       if (x == 0 || x < -1) {
          fprintf(stderr, "Warning: reading from file into send buffer either finished or failed. Number of bytes read: %d\n", numbytes);
          break;
       }
       numbytes += x;
    }

    ls = lseek(fileno(stream), 0, SEEK_SET);
    if (ls == -1) {
       fprintf(stderr, "Error: seeking to requested chunk location failed. File buffer pointer at unknown location.\n");
    }

    mftp_packet p;
    p.seq = seq;
    p.flag = DATA;
    bzero(p.data, 1024);
    memcpy(p.data, buffer, sizeof(buffer));
    if (f_offset == 0) 
    DEBUGF("%s", p.data);
    return p;
}


void debugprintf(char *format, ...) {
   va_list args;
   fflush (NULL);
   va_start (args, format);
   fprintf (stdout, "DEBUG: ");
   vfprintf (stdout, format, args);
   va_end (args);
   fflush (NULL);
}

