// File: utils.h    
// Created April 7, 2014
// Michael Baptist - mbaptist@ucsc.edu

#ifndef __UTILS_H__
#define __UTILS_H__

#include "rudp.h"

//#define NDEBUG NoDebug


/**
 * @file client_utils.h
 * My utitlies header file for checking errors for system and api function calls.
 */

/**
 * A boolean data type created by an enum.
 */
typedef enum {FALSE = 0, TRUE = 1} bool;

/**
 * Checks the return value of the socket(3) networking api call for any errors and prints messages and sets the exit status accordingly.
 *
 * @param fd The file descriptor returned by the socket(3) call.
 */
void check_socket(int fd);

/**
 * Checks the return value of the connect(3) networking api call for any errors and prints messages and sets the exit status accordingly.
 * 
 * @param val The return value from the connect(3) call.
 */
void check_connection(int val);

/**
 * Checks the current directory for the file filename. If file name is found retrieve_file will attepmt to open the file using fopen. If not an error message will be returned. 
 *
 * @param filename The file to be searched for and opened.
 * @param mode The mode in which the file will be opened.
 *
 * @return The file descriptor for the file if fopen succeeds. Otherwise NULL is returned if filename is not found, or if fopen fails.
 */
FILE *retrieve_file(const char *restrict filename, const char *restrict mode);

/**
 * Takes a file and gives back the size of the file. 
 *
 * @param filename The file in which  you want the size of.
 *
 * @return The size of the file filename, or -1 if an error occurs. Errno will be set to the proper error.
 */
int get_file_size(FILE *restrict filename);

/**
 * Gets a chunk of a file to a client socket.
 *
 * @param f_offset The offset to index into the file.
 * @param stream The file to read from to send the chunk.
 * @param seq The sequence number.
 *
 */
mftp_packet get_file_chunk(int f_offset, FILE *restrict stream, int seq);

/**
 * Allows for debugging print statements to be made and easily turned off for release build
 *
 * @param format The format string to format the print statement.
 */
#ifdef NDEBUG
#define DEBUGF(...) // DEBUG (__VA_ARGS__)
#else
#define DEBUGF(...) debugprintf (__VA_ARGS__)
void debugprintf (char *format, ...);
#endif

#endif

