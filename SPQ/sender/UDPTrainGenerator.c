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
** ./unitExperimentSender num_of_packets probe_payload_length high_priority_address low_priority_address priority
** Example: ./sender 60000 1500 127.0.0.1 127.0.0.2 H
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
error_t UDPTrainGenerator (int initial_train_length, int seperation_train_length, int num_packet_trains, 
  int probe_payload_length, char* high_priority_address, char* low_priority_address, char priority)
{
  // Set up high priority send_socket
  struct addrinfo hints_hi;
  memset(&hints_hi, 0, sizeof hints_hi);
  hints_hi.ai_family = AF_INET;
  hints_hi.ai_socktype = SOCK_DGRAM;

  //Fill data structure with address and port number of high priority destination
  struct addrinfo* dest_addr_info_hi;
  int status = getaddrinfo(high_priority_address, UDP_PROBE_PORT_NUMBER, &hints_hi, &dest_addr_info_hi);
  if (status != 0)
  {
    fprintf(stderr, "ERROR #%d: Address Information Error\n", ADDRINFO_ERROR);
    return ADDRINFO_ERROR;
  }

  // Set up low priority send_socket
  struct addrinfo hints_low;
  memset(&hints_low, 0, sizeof hints_low);
  hints_low.ai_family = AF_INET;
  hints_low.ai_socktype = SOCK_DGRAM;

  //Fill data structure with address and port number of low priority destination
  struct addrinfo* dest_addr_info_low;
  status = getaddrinfo(low_priority_address, UDP_PROBE_PORT_NUMBER, &hints_low, &dest_addr_info_low);
  if (status != 0)
  {
    fprintf(stderr, "ERROR #%d: Address Information Error\n", ADDRINFO_ERROR);
    return ADDRINFO_ERROR;
  }

  //Set up socket to send packets to host with udp for high priority packets
  int send_socket_hi = socket(dest_addr_info_hi->ai_family, dest_addr_info_hi->ai_socktype, dest_addr_info_hi->ai_protocol);
  if (send_socket_hi == -1)
  {
    fprintf(stderr, "ERROR #%d: Socket Setup Error\n", SOCKET_SETUP_ERROR);
    return SOCKET_SETUP_ERROR;
  }

  //Set up socket to send packets to host with udp for low priority packets
  int send_socket_low = socket(dest_addr_info_low->ai_family, dest_addr_info_low->ai_socktype, dest_addr_info_low->ai_protocol);
  if (send_socket_low == -1)
  {
    fprintf(stderr, "ERROR #%d: Socket Setup Error\n", SOCKET_SETUP_ERROR);
    return SOCKET_SETUP_ERROR;
  }

  // Set up packet_data and fill it with zeros
  uint8_t* packet_data_first;
  packet_data_first = (uint8_t*) calloc (probe_payload_length, 1);
  uint8_t* packet_data_remaining;
  packet_data_remaining = (uint8_t*) calloc (probe_payload_length, 1);
    

  //Set the first packet sequence number
  int packet_seq_id = 0;
  *((int*) packet_data_first) = packet_seq_id;

  //Fill the rest of the packets with packet id and timestamp
  //struct timespec currentTime, lastSendTime;
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastSendTime);   //It might be removed since it is no longer being used

  //The priority switch allows for sending two types of packets
  //Per train based on priority. For High priority, it sends one low priority backet followed by seperation_train_length-1 high priority packets

  struct addrinfo* first_packet_dest = 0;
  struct addrinfo* remaining_packet_dest = 0;
  int first_socket = 0;
  int remaining_socket = 0;
  if(priority == 'L'){
    first_packet_dest = dest_addr_info_low;
    remaining_packet_dest = dest_addr_info_hi;
    first_socket = send_socket_low;
    remaining_socket = send_socket_hi;
  }
  else if(priority == 'H'){
    first_packet_dest = dest_addr_info_hi;
    remaining_packet_dest = dest_addr_info_low;
    first_socket = send_socket_hi;
    remaining_socket = send_socket_low;
  }
  else{
    //SHOULD NOT GO HERE
  }

  //Send initial packet train of High Priority
  int num_packets_sent = 0;
  while (num_packets_sent < initial_train_length)
  {
    //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &currentTime); //It might be removed since it is no longer being used
    sendto(send_socket_hi, packet_data_first, probe_payload_length, 0,
    dest_addr_info_hi->ai_addr, dest_addr_info_hi->ai_addrlen);
    //lastSendTime = currentTime;  //It might be removed since it is no longer being used
    packet_seq_id++;
    *((int*) packet_data_first) = packet_seq_id;

    num_packets_sent++;
  }

  //Sequence Id for High and Low are separate
  int first_packet_seq_id = 0;
  int remaining_packet_seq_id = 0;
  *((int*) packet_data_first) = first_packet_seq_id;
  *((int*) packet_data_remaining) = remaining_packet_seq_id;

  //Send prioritized packet trains based on prioirty parameter
  int num_trains_sent = 0;
  while(num_trains_sent < num_packet_trains){
    num_packets_sent = 0;
    sendto(first_socket, packet_data_first, probe_payload_length, 0,
      first_packet_dest->ai_addr, first_packet_dest->ai_addrlen);
    first_packet_seq_id++;
    *((int*) packet_data_first) = first_packet_seq_id;
    while (num_packets_sent < seperation_train_length)
    {
      //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &currentTime); //It might be removed since it is no longer being used
      sendto(remaining_socket, packet_data_remaining, probe_payload_length, 0, 
        remaining_packet_dest->ai_addr, remaining_packet_dest->ai_addrlen);
      //lastSendTime = currentTime;  //It might be removed since it is no longer being used
      remaining_packet_seq_id++;
      *((int*) packet_data_remaining) = remaining_packet_seq_id;

      num_packets_sent++;
    }
    num_trains_sent++;
  }
  

  //Free structs, ptrs, and close socket
  freeaddrinfo (dest_addr_info_hi);
  freeaddrinfo (dest_addr_info_low);
  free (packet_data_first);
  free (packet_data_remaining);
  close (send_socket_hi);
  close (send_socket_low);

  return SUCCESS;
}


int main(int argc, char *argv[])
{

  //Sender takes in 5 arguments
  //./unitExperimentSender num_of_packets probe_payload_length high_priority_address low_priority_address priority
  if(argc != 8)
  {
    fprintf(stderr,"ERROR #%d: INVALID NUMBER OF ARGUMENTS\n", INVALID_NUMBER_OF_ARGUMENTS);
    fprintf(stderr, "Usage: ./unitExperimentSender initial_train_length seperation_train_length num_packet_trains probe_payload_length high_priority_address low_priority_address priority\n");
    return INVALID_NUMBER_OF_ARGUMENTS;
  }
  int initial_train_length = atoi(argv[1]); //Number of Initial High Priority Packets
  int seperation_train_length = atoi(argv[2]); //Number of Packets Per Train
  int num_packet_trains = atoi(argv[3]); //Number of Packet Trains
  int probe_payload_length = atoi(argv[4]); //[0,1500] in bytes
  char* high_priority_address = argv[5]; //ip address of compression node X??.X??.X??.X??   TODO? must be IPv4 address?
  char* low_priority_address = argv[6];
  char* priority = argv[7];//priority either 'H' or 'L'

  /*Call UDP Connection to Send Data to Receiver*/
  if(UDPTrainGenerator(initial_train_length, seperation_train_length, num_packet_trains, probe_payload_length, high_priority_address, low_priority_address, priority[0]) != SUCCESS)
  {
    fprintf(stderr, "ERROR #%d: UDP Train Generator Error\n", UDP_TRAIN_GENERATOR_FAILED);
    return UDP_TRAIN_GENERATOR_FAILED;
  }

  return SUCCESS;
}

