/**************************************************************************
** UDP Sender Code
** Generate a packet train that interleaves packets based on priority
** of the packet.
**
** Initial Packets:
**   Send Initial_Train_Length of high priority packets for all
**   experiments.
** 
** If High Priority: 
**   Send a single high priority packet followed by
**   Seperation_Train_Length of low prioity packets
**
** If Low Priority:
**   Send a single low priority packet followed by
**   Seperation_Train_Length of high prioity packets
** 
** Parameters:
**  (1) initial_train_length = Number of Initial High Priority Packets
**  (2) seperation_train_length = Number of Packets Per Train
**  (3) num_packet_trains = Number of Packet Trains
**  (4) probe_payload_length = Size of each packet. [0,1500] in bytes.
**  (5) receiver_address = ip4 address of compression node X??.X??.X??.X??
**  (6) priority = priority either 'H' or 'L'
**
** How To Run Code:
**   ./unitExperimentSender initial_train_length seperation_train_length 
**   num_packet_trains probe_payload_length receiver_address priority
**
** Example: 
**   ./unitExperimentSender 2001 19 200 100 131.179.192.60 H
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
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
error_t UDPTrainGenerator (int initial_train_length, int seperation_train_length, int num_packet_trains, 
  int probe_payload_length, char* receiver_address, char priority)
{
  // Set up high priority send_socket
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  //Fill data structure with address and port number of high priority destination
  struct addrinfo* dest_addr_info;
  int status = getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER_HIGH, &hints, &dest_addr_info);
  if (status != 0)
  {
    fprintf(stderr, "ERROR #%d: Address Information Error\n", ADDRINFO_ERROR);
    return ADDRINFO_ERROR;
  }

  //Set up socket to send packets to host with udp for high priority packets
  int send_socket = socket(dest_addr_info->ai_family, dest_addr_info->ai_socktype, dest_addr_info->ai_protocol);
  if (send_socket == -1)
  {
    fprintf(stderr, "ERROR #%d: Socket Setup Error\n", SOCKET_SETUP_ERROR);
    return SOCKET_SETUP_ERROR;
  }

  // Set up packet_data and fill it with zeros
  uint8_t* packet_data;
  packet_data = (uint8_t*) calloc (probe_payload_length, 1);
  uint8_t* packet_data_low;
  packet_data_low = (uint8_t*) calloc (probe_payload_length, 1);
    

  //Set the first packet sequence number
  int packet_seq_id_high = 0;
  int packet_seq_id_low = 0;
  *((int*) packet_data) = packet_seq_id_high;
  *((int*) packet_data_low) = packet_seq_id_low;
  ((char*) packet_data)[4] = 'H';
  ((char*) packet_data_low)[4] = 'L';

  //Send initial packet train of High Priority
  int num_packets_sent = 0;
  while (num_packets_sent < initial_train_length)
  {
    sendto(send_socket, packet_data, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
    packet_seq_id_high++;
    *((int*) packet_data) = packet_seq_id_high;

    num_packets_sent++;
  }

  //Send prioritized packet trains based on prioirty parameter
  //int zero_tos = 0;
  if( priority == 'L'){
    int num_trains_sent = 0;
    while(num_trains_sent < num_packet_trains){
      num_packets_sent = 0;

      getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER_LOW, &hints, &dest_addr_info);
      sendto(send_socket, packet_data_low, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
      packet_seq_id_low++;
      *((int*) packet_data_low) = packet_seq_id_low;

      getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER_HIGH, &hints, &dest_addr_info);

      while (num_packets_sent < seperation_train_length)
      {
        sendto(send_socket, packet_data, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
        packet_seq_id_high++;
        *((int*) packet_data) = packet_seq_id_high;
        num_packets_sent++;  
      }
      num_trains_sent++;
    }
  }
  else if( priority == 'H'){
    int num_trains_sent = 0;
    while(num_trains_sent < num_packet_trains){
      num_packets_sent = 0;

      getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER_HIGH, &hints, &dest_addr_info);
      sendto(send_socket, packet_data, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
      packet_seq_id_high++;
      *((int*) packet_data) = packet_seq_id_high;

      getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER_LOW, &hints, &dest_addr_info);
      while (num_packets_sent < seperation_train_length)
      {
        sendto(send_socket, packet_data_low, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
        packet_seq_id_low++;
        *((int*) packet_data_low) = packet_seq_id_low;
        num_packets_sent++;  
      }
      num_trains_sent++;
    }
  }  

  //Free structs, ptrs, and close socket
  freeaddrinfo (dest_addr_info);
  free (packet_data);
  free (packet_data_low);
  close (send_socket);

  return SUCCESS;
}


int main(int argc, char *argv[])
{

  //Sender takes in 6 arguments
  // ./unitExperimentSender initial_train_length seperation_train_length 
  // num_packet_trains probe_payload_length receiver_address priority
  if(argc != 7)
  {
    fprintf(stderr,"ERROR #%d: INVALID NUMBER OF ARGUMENTS\n", INVALID_NUMBER_OF_ARGUMENTS);
    fprintf(stderr, "Usage: ./unitExperimentSender initial_train_length seperation_train_length num_packet_trains probe_payload_length receiver_address priority\n");
    return INVALID_NUMBER_OF_ARGUMENTS;
  }
  int initial_train_length = atoi(argv[1]); //Number of Initial High Priority Packets
  int seperation_train_length = atoi(argv[2]); //Number of Packets Per Train
  int num_packet_trains = atoi(argv[3]); //Number of Packet Trains
  int probe_payload_length = atoi(argv[4]); //[0,1500] in bytes
  char* receiver_address = argv[5]; //ip address of compression node X??.X??.X??.X??   TODO? must be IPv4 address?
  char* priority = argv[6];//priority either 'H' or 'L'

  /*Call UDP Connection to Send Data to Receiver*/
  if(UDPTrainGenerator(initial_train_length, seperation_train_length, num_packet_trains, probe_payload_length, receiver_address, priority[0]) != SUCCESS)
  {
    fprintf(stderr, "ERROR #%d: UDP Train Generator Error\n", UDP_TRAIN_GENERATOR_FAILED);
    return UDP_TRAIN_GENERATOR_FAILED;
  }

  return SUCCESS;
}

