// File: rudp.h    
// Created May 8, 2014
// Michael Baptist - mbaptist@ucsc.edu

#ifndef __RUDP_H__
#define __RUDP_H__

// uncomment to turn off debugging.
//#define NDEBUG NoDebug

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

/**
 * Flags for custom packet.
 */
#define START 1
#define DATA  2
#define ACK   3
#define ERROR 4

/**
 * My custom protocol packet.
 */
typedef struct mftp_packet {
    unsigned int seq;           // sequence number
    char data[1024];    // packet data.
    unsigned int flag;          // flag for type of data.
} mftp_packet;
typedef mftp_packet *mftp_packet_ref;


/**
 * Serializes an int into a unsigned char
 * 
 * @param buffer The array to insert the data.
 * @param val The value to serialize.
 *  
 * @return A pointer to the next free space in the buffer. 
 */
unsigned char *serialize_int(unsigned char *buffer, unsigned int val);

/**
 * Serializes an char array into a unsigned char array
 * 
 * @param buffer The array to insert the data.
 * @param buf The value to serialize.
 * @param len Then length of buf.
 *  
 * @return A pointer to the next free space in the buffer. 
 */
unsigned char *serialize_data(unsigned char *buffer, char buf[], int len);


/**
 * Deserializes an int into a unsigned char
 * 
 * @param buffer The array to get the data out of.
 * @param val The value to save the data.
 *  
 * @return A pointer to the next free space in the buffer. 
 */
unsigned char *deserialize_int(unsigned char *buffer, unsigned int *val);


/**
 * Deserializes an char array into a unsigned char array
 * 
 * @param buffer The array to get the data from.
 * @param buf The buffer to save it.
 * @param len Then length of buf.
 *  
 * @return A pointer to the next free space in the buffer. 
 */
unsigned char *deserialize_data(unsigned char *buffer, char buf[], int len);

/**
 * Serialize mftp_packet into a buffer
 * 
 * @param packet The packet to be serialized into a buffer.
 * @param buffer The buffer to fill up/
 * @param len The length of the buffer.
 */
unsigned char *serialize_packet(mftp_packet packet, unsigned char buffer[]);

/**
 * Deserialize buffer into mftp_packet
 * 
 * @param buffer The buffer to get data from.
 * @param len The length of the buffer.
 */
mftp_packet deserialize_packet(unsigned char buffer[]);

/**
 * Send an ack datagram to a socket.
 *
 * @param sequence_number The sequence number of the datagram
 * @param clisock The socket to send the data to. 
 * @param client The reciever.
 * @param clen The length of the sockaddr_in struct.
 */
void send_ack(int sequence_number, int clisock, sockaddr_in client, int clen);

/**
 * Send an error datagram to a socket.
 *
 * @param sequence_number The sequence number of the datagram
 * @param clisock The socket to send the data to. 
 * @param client The reciever.
 * @param clen The length of the sockaddr_in struct.
 */
void send_error(int sequence_number, int clisock, sockaddr_in client, int clen);

/**
 * Sends a datagram to a client.
 *
 * @param socket The socket to send to.
 * @param cli the structure with the ip and port to send to. 
 * @param dlen length of the stucture cli.
 * @param data The custom packet with data and flags etc.
 *
 * @return Returns 1 if successful and 0 if it fails. Approriate messages are printed to stderr.
 */
int send_dgram(int socket, const struct sockaddr_in *cli, int dlen, const mftp_packet data);

/**
 * Parses incoming datagrams. runs the deserializer.
 *
 * @param buffer The buffer to parse into a mftp struct
 *
 * @return A mftp_packet filled with data.
 */
mftp_packet parse_dgram(unsigned char buffer[]);

#endif
