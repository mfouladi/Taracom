/******************************************************************
** UDP Sender Code
** Generate a packet train with either high or low entropy packets
** that sends its packets without delay between packets to the 
** destination address using UDP. Each packets within a packet train
** holds a sequence id number starting from 0 to the number of
** packets minus 1.
** Low entropy is has a packet load of all zeros. High entropy is 
** read from dev/urandom.
**
** Single Packet Structure:
**      
**    0                   1                   2   
**    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 . . . n-1
**    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**    |  ID |        High or Low Entropy Data         |
**    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**
** How To Run Code:
** ./sender num_packets payload_length compression_node_addr entropy
** Example: ./sender 60000 1500 127.0.0.1 H
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

#include "taracomConstants.h"

/***************************************************************
 * Function used to return a timespec that holds the difference
 * between start and end times in both seconds and nanoseconds
 ***************************************************************/
struct timespec diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec) < 0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return temp;
}

/***************************************************************
 * This is the main function of the file.
 * It creates the packet with a given entropy and sends it to the
 * the compression node address. 
 ***************************************************************/
error_t UDPTrainGenerator (int num_of_packets,int probe_payload_length,char entropy,char* compression_node_addr)
{
  // Set up send_socket
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  //Fill data structure with address and port number of destination
  struct addrinfo* dest_addr_info;
  int status = getaddrinfo(compression_node_addr, UDP_PROBE_PORT_NUMBER, &hints, &dest_addr_info);
  if (status != 0)
  {
    fprintf(stderr, "ERROR #%d: Address Information Error\n", ADDRINFO_ERROR);
    return ADDRINFO_ERROR;
  }

  //Set up socket to send packets to host with udp
  int send_socket = socket(dest_addr_info->ai_family, dest_addr_info->ai_socktype, dest_addr_info->ai_protocol);
  if (send_socket == -1)
  {
    fprintf(stderr, "ERROR #%d: Socket Setup Error\n", SOCKET_SETUP_ERROR);
    return SOCKET_SETUP_ERROR;
  }

  // Set up packet_data
  // if low entropy, fill data with 0
  // if high entropy, fill data with data from /dev/urandom
  uint8_t* packet_data;
  if (entropy == 'L'){
    packet_data = (uint8_t*) calloc (probe_payload_length, 1);
  }
  else if (entropy == 'H'){
    //allocate memory for the buffer
    packet_data = (uint8_t*) malloc (probe_payload_length);

    //Open up random number generator
    FILE* urandom;
    urandom = fopen("/dev/urandom", "r");
    if (urandom == NULL)
    {
      fprintf(stderr, "ERROR #%d: Reading From DevRandom Error\n", DEVRANDOM_ERROR);	
      return DEVRANDOM_ERROR;
    }

    //Write from the random number generator into the buffer
    if( fread(packet_data, probe_payload_length, 1, urandom) != 1){
      fprintf(stderr, "ERROR #%d: FREAD_ERROR\n", ENTROPY_PARAM_ERROR);	
      fclose(urandom);
      return DEVRANDOM_ERROR;
    }
    //close random number generator
    fclose(urandom);
  }
  else{ 
    //Should not happen
    fprintf(stderr, "ERROR #%d: Invalid Entropy Type (H/L) Error\n", ENTROPY_PARAM_ERROR);	
    return ENTROPY_PARAM_ERROR;
  }
    

  //Set the first packet sequence number
  int packet_seq_id = 0;
  *((int*) packet_data) = packet_seq_id;

  //Fill the rest of the packets with packet id and timestamp
  //struct timespec currentTime, lastSendTime;
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastSendTime);   //It might be removed since it is no longer being used

  while (packet_seq_id < num_of_packets)
  {
    //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &currentTime); //It might be removed since it is no longer being used
    sendto(send_socket, packet_data, probe_payload_length, 0,
    dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
    //lastSendTime = currentTime;  //It might be removed since it is no longer being used
    packet_seq_id++;
    *((int*) packet_data) = packet_seq_id;
  }

  //Free structs, ptrs, and close socket
  freeaddrinfo (dest_addr_info);
  free (packet_data);
  close (send_socket);

  return SUCCESS;
}


int main(int argc, char *argv[])
{

  //Sender takes in 4 arguments
  //./unitExperimentSender num_of_packets probe_payload_length compression_node_addr entropy
  if(argc != 5)
  {
    fprintf(stderr,"ERROR #%d: INVALID NUMBER OF ARGUMENTS\n", INVALID_NUMBER_OF_ARGUMENTS);
    fprintf(stderr, "Usage: ./unitExperimentSender num_of_packets probe_payload_length compression_node_addr entropy\n");
    return INVALID_NUMBER_OF_ARGUMENTS;
  }
  int num_of_packets = atoi(argv[1]); //Number of Packets [1,60000]
  int probe_payload_length = atoi(argv[2]); //[0,1500] in bytes
  char* compression_node_addr = argv[3]; //ip address of compression node X??.X??.X??.X??   TODO? must be IPv4 address?
  char* entropy = argv[4];//entropy either 'H' or 'L'

  /*Call UDP Connection to Send Data to Receiver*/
  if(UDPTrainGenerator(num_of_packets, probe_payload_length,entropy[0],compression_node_addr) != SUCCESS)
  {
    fprintf(stderr, "ERROR #%d: UDP Train Generator Error\n", UDP_TRAIN_GENERATOR_FAILED);
    return UDP_TRAIN_GENERATOR_FAILED;
  }

  return SUCCESS;
}

