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
** ./unitExperimentSender num_of_packets probe_payload_length receiver_address receiver_address priority
** Example: ./sender 60000 1500 127.0.0.1 127.0.0.2 H
********************************************************************/
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
  int status = getaddrinfo(receiver_address, UDP_PROBE_PORT_NUMBER, &hints, &dest_addr_info);
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

  //Set IP TOS to low delay
  int lowdelay = IPTOS_LOWDELAY;
  if (setsockopt(send_socket, IPPROTO_IP, IP_TOS, (void *)&lowdelay, sizeof(lowdelay)) < 0){
    fprintf(stderr, "ERROR #%d: Socket TOS Error\n", SOCKET_SETUP_ERROR);
    return SOCKET_SETUP_ERROR;
  }

  // Set up packet_data and fill it with zeros
  uint8_t* packet_data_high;
  packet_data_high = (uint8_t*) calloc (probe_payload_length, 1);
  uint8_t* packet_data_low;
  packet_data_low = (uint8_t*) calloc (probe_payload_length, 1);
    

  //Set the first packet sequence number
  int packet_seq_id = 0;
  *((int*) packet_data_high) = packet_seq_id;

  //Send initial packet train of High Priority
  int num_packets_sent = 0;
  while (num_packets_sent < initial_train_length)
  {
    sendto(send_socket, packet_data_high, probe_payload_length, 0,
    dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
    packet_seq_id++;
    *((int*) packet_data_high) = packet_seq_id;

    num_packets_sent++;
  }

  //Sequence Id for High and Low are separate
  int high_packet_seq_id = packet_seq_id;
  int low_packet_seq_id = 0;
  *((int*) packet_data_high) = high_packet_seq_id;
  *((int*) packet_data_low) = low_packet_seq_id;

  //Send prioritized packet trains based on prioirty parameter
  int zero_tos = 0;
  if( priority == 'L'){
    int num_trains_sent = 0;
    while(num_trains_sent < num_packet_trains){
      num_packets_sent = 0;
      setsockopt(send_socket, IPPROTO_IP, IP_TOS, (void *)&zero_tos, sizeof(zero_tos));
      sendto(send_socket, packet_data_low, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
      low_packet_seq_id++;
      *((int*) packet_data_low) = low_packet_seq_id;
      setsockopt(send_socket, IPPROTO_IP, IP_TOS, (void *)&lowdelay, sizeof(lowdelay));
      while (num_packets_sent < seperation_train_length)
      {
        sendto(send_socket, packet_data_high, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
        high_packet_seq_id++;
        *((int*) packet_data_high) = high_packet_seq_id;
        num_packets_sent++;  
      }
      num_trains_sent++;
    }
  }
  else if( priority == 'H'){
    int num_trains_sent = 0;
    while(num_trains_sent < num_packet_trains){
      num_packets_sent = 0;
      setsockopt(send_socket, IPPROTO_IP, IP_TOS, (void *)&lowdelay, sizeof(lowdelay));
      sendto(send_socket, packet_data_high, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
      high_packet_seq_id++;
      *((int*) packet_data_high) = high_packet_seq_id;
      setsockopt(send_socket, IPPROTO_IP, IP_TOS, (void *)&zero_tos, sizeof(zero_tos));
      while (num_packets_sent < seperation_train_length)
      {
        sendto(send_socket, packet_data_low, probe_payload_length, 0, dest_addr_info->ai_addr, dest_addr_info->ai_addrlen);
        low_packet_seq_id++;
        *((int*) packet_data_low) = low_packet_seq_id;
        num_packets_sent++;  
      }
      num_trains_sent++;
    }
  }
  
  

  //Free structs, ptrs, and close socket
  freeaddrinfo (dest_addr_info);
  free (packet_data_high);
  free (packet_data_low);
  close (send_socket);

  return SUCCESS;
}


int main(int argc, char *argv[])
{

  //Sender takes in 5 arguments
  //./unitExperimentSender num_of_packets probe_payload_length receiver_address receiver_address priority
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

