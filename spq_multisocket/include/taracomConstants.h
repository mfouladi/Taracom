#ifndef TARACOMCONSTANTS_H
#define TARACOMCONSTANTS_H

/* Globally Defined Send Buffer Length*/
#define MAX_SEND_BUFFER_SIZE 300000
#define MAX_PACKET_SIZE 10000
//#define IP_ADDRESS_LENGTH  15
#define POST_TCP_SERVER_PORT 26400
#define PRE_TCP_SERVER_PORT 16400
#define UDP_PROBE_PORT_NUMBER "9876"


//Set to 0 to turn off debugging and 1
//to turn on debugging
#define DEBUG_MODE 0

//UINT MAX
//#define UINT16_MAX (65535U)

//Max Message Array Size
#define MAX_NUMBER_OF_PACKETS 10000
#define MAX_FILENAME_SIZE 256
#define IP_ADDRESS_LENGTH 16

//Error Message Type Definition
typedef int error_t;

//General Success and Failure Messages
#define SUCCESS 0 /*Function ran as intended*/
#define FAILURE 1 /*Function failed to run as intended*/

//Other General Error Mesages
#define INVALID_NUMBER_OF_ARGUMENTS 11 /*Missing or Extra arguments to the function*/
#define INVALID_ENTROPY_PARAMETER 	12 /*Entropy was not 'H' or 'L'*/
#define INVALID_PORT_NUMBER 		13 /*Port number was out of valid range*/
#define INVALID_IP_ADDRESS 			14 /*IP Address was formatted incorrectly*/
#define SOCKET_CREATION_FAILURE     15 /*Failed to create socket*/
#define INVALID_FILENAME			16

//PREVIUOSLY DEFINED ERRORS
#define UDP_TRAIN_GENERATOR_FAILED  100
#define UDP_TRAIN_RECEIVER_FAILED   101
#define SOCKET_SETUP_ERROR          102
#define CONNECT_ERROR               103
#define SEND_ERROR                  104
#define RECEIVE_ERROR               105
#define BIND_ERROR                  106
#define LISTEN_ERROR                107
#define ACCEPT_CLIENT_ERROR         108
#define ADDRINFO_ERROR              109
#define DEVRANDOM_ERROR             110
#define ENTROPY_PARAM_ERROR         111
#define COMPRESSION_ERROR           112
#define DECOMPRESSION_ERROR         113
#define FREAD_ERROR                 114
#define FWRITE_ERROR				115
#define FILE_ERROR			      	116

//Sender Specific Errors
#define URANDOM_FILE_OPEN_FAILED 	201 /*Failed to open the dev/random file*/
#define URANDOM_READ_ERROR 			202 /*Failed while reading from dev/random file*/
#define SENDER_CONNECTION_ERROR 	203 /*Error creating connection from sender*/
#define WRITE_TO_SERVER_ERROR 		204 /*Error sending packets to host server*/

//RECEIVER SPECIFIC ERRORS
#define HOST_BINDING_ERROR			301 /*Failed to bind host to socket*/

#endif
