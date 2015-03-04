/*************************************************************
** UDP Receiver Code.
** Set up a timed experiment on a specific port that will receive packets
** for a set experiment time and then write all to file when the experiment
** time has ended.
** Receives datagrams from sender of sequenced packets that have a packet id
** and will time stamp these packets and store the id and the time stamp
** in a file at the end of the experiment
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "taracomConstants.h"
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>


//Set to 0 to turn off debugging and 1
//to turn on debugging
#define DEBUG_MODE 0
#define VERBOSE 0


/*
 * Function Used to return a timespec that holds the difference
 * between start and end times in both seconds and nanoseconds
 */
struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
        if ((end.tv_nsec-start.tv_nsec)<0) {
                temp.tv_sec = end.tv_sec-start.tv_sec-1;
                temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
        } else {
                temp.tv_sec = end.tv_sec-start.tv_sec;
                temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
        return temp;
}

int recv_socket;


void handle_shutdown(int sig)
{
        if (recv_socket)
        	close(recv_socket);
        pthread_exit(NULL);

}

error_t UDPTrainReceiver (char* buffer, int probe_packet_length, unsigned long initial_experiment_run_time, 
	unsigned long later_experiment_run_time, unsigned long inter_experiment_sleep_time)
{
	
	//Not sure what this does exactly
	if (signal(SIGINT, handle_shutdown) == SIG_ERR)
		fprintf(stderr, "Can’t set the interrupt handler");

	//SET UP FOR THE SOCKET
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;

	//Set to Datagram packets (UDP)
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo* my_addr_info;

	//Get information about host
	int status = getaddrinfo(NULL, UDP_PROBE_PORT_NUMBER, &hints, &my_addr_info);
	if (status != 0)
	{
		fprintf(stderr, "ERROR #%d: Address Information Error", ADDRINFO_ERROR);
		return ADDRINFO_ERROR;
	}

	//Create the UDP socket file descriptor
	int recv_socket = socket(my_addr_info->ai_family, my_addr_info->ai_socktype, my_addr_info->ai_protocol);

	// Set recv_socket to not block
	fcntl(recv_socket, F_SETFL, O_NONBLOCK);
	if (recv_socket == -1)
	{
		fprintf(stderr, "ERROR #%d: Socket Setup Error", SOCKET_SETUP_ERROR);
		return SOCKET_SETUP_ERROR;
	}

	//Set up socket so it can be reused without failing
	int reuse = 1;
	if (setsockopt(recv_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
	{
		fprintf(stderr, "Can't set the reuse option on the socket");
		return SOCKET_SETUP_ERROR;
	}

	//Bind the socket to an address and a port
	status = bind(recv_socket, my_addr_info->ai_addr, my_addr_info->ai_addrlen);
	if (status == -1)
	{
		fprintf(stderr, "ERROR #%d: Binding Error", BIND_ERROR);
		close (recv_socket);
		return BIND_ERROR;
	}

	//Free the space for the initialized address info, it is not longer needed
	freeaddrinfo(my_addr_info);

	// SET UP EXPERIMENT VARIABLES

	//Set to 1 when sender ip address is known
	int have_ip_address = 0;
	
	//Used to receive packets
	char temp[100];

	//Used to store the packet before writing to file
	char packet_buffer[2000];

	//Deliminator used to separate experiments in the output file
	const char delim[4] = "*\n";
	int delim_size = strlen(delim);

	//Output file name extension
	const char extension[5] = ".raw";

	int buf_len = 0;
	int log_size;
	int recv_bytes;

	//Use to identify sequence order
	int current_seq_id;
	//int last_seq_id = -1;
	char priority;

	//Used to determine sender IP
	struct sockaddr_in from_addr;
	socklen_t from_len = sizeof(from_addr);

	//Used for timing of experiment
	unsigned long experiment_run_time = initial_experiment_run_time;
	struct timespec lastWrite, currentTime, ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastWrite);
	currentTime = lastWrite;

	if(VERBOSE) printf("waiting for data...\n");

	while (true)
    {
    	//If recv_socket is null, return error
		if(!recv_socket)
		{
			fprintf(stderr, "recv_socket ended loop\n");		
			return RECEIVE_ERROR;
		}

		//If a packet has already been received, then continue to receive from that IP address
		//else wait to set up IP address information and set established address to 1 for future
		//receives
		if (have_ip_address)
		{
			recv_bytes = recvfrom(recv_socket, packet_buffer, probe_packet_length-1, 0, NULL, NULL);
		}
		else 
		{
			if ((recv_bytes = recvfrom(recv_socket, packet_buffer, probe_packet_length-1, 0, (struct sockaddr*) &from_addr, &from_len))>0)
			{
				have_ip_address = 1;
			}
		}


		//Get current clock time for possible interrupt and to 
		//mark the received packets
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &currentTime);

		//If a valid packet was received, 
		if (recv_bytes > 0)
  		{
  			//Get the time elapsed since last write to buffer (i.e. the arrival time of the last successful (not-lost) packet. 
			ts = diff(lastWrite, currentTime);

			//Get the packet sequence number
			current_seq_id = *((int*) (packet_buffer));
			priority = ((char*) packet_buffer)[4];


			//if(VERBOSE) printf("%d\n", current_seq_id);

			// If this is the next packet, write packet ID and current 
			//time to a temporary buffer
			//if (current_seq_id - last_seq_id == 1)
    	    //{
				log_size = sprintf(temp, "%d\t%c\t%d.%.9ld\n",current_seq_id, priority, 
				(int) ts.tv_sec, ts.tv_nsec);
				memcpy ((void*) (buffer + buf_len), (void*) temp, log_size);
				buf_len += log_size;
				//last_seq_id = current_seq_id;
	    	//}
			// If some packets were skipped, assume lost
			// and fill the gap for the missing packets by
			// writing the sequence ID along with -1 to indicate
			// a lost packet
			/*else if (current_seq_id - last_seq_id > 1)
			{
				int i;
				for (i=last_seq_id+1; i<current_seq_id; i++)
				{
					log_size = sprintf(temp, "%d\t-1\n", i);
					memcpy ((void*) (buffer + buf_len), (void*) temp, log_size);
					buf_len += log_size;
				}

				//Write the current sequence id to the buffer
				log_size = sprintf(temp, "%d\t%d.%.9ld\n",current_seq_id, (int) ts.tv_sec, ts.tv_nsec);
				memcpy ((void*) (buffer + buf_len), (void*) temp, log_size);
				buf_len += log_size;
				last_seq_id = current_seq_id;
			}
			// If this is the start of a new train
			else if (current_seq_id <= 5)   //TODO? hmmm might be ok for compression but ...
			{
				//Write the delimiter to the buffer and increment buffer by size of delim
				memcpy ((void*) (buffer + buf_len), (void*) delim, delim_size);
				buf_len += delim_size;
				log_size = sprintf(temp, "%d\t%d.%.9ld\n",current_seq_id, (int) ts.tv_sec, ts.tv_nsec);

				//Write the temporary array to the buffer and increment buffer by temp size
				memcpy ((void*) (buffer + buf_len), (void*) temp, log_size);
				buf_len += log_size;
				last_seq_id = current_seq_id;
			}
			*/
        }

        // if it has been longer than experiment run time
        // (in other words, all packets of the train should have been received by now)
		// Check: Is it time to write to the output file yet?
		if (diff(lastWrite, currentTime).tv_sec >=  experiment_run_time)
		{
			//Reset the previous time to the current time
			lastWrite = currentTime;

			//Schedule the next write to file
			experiment_run_time = later_experiment_run_time;  //which is half of the initial_experiment_run_time

			//Variable to hold the output file name
			char file_name[50] = "./temp/";

			//If we know the sender ip address, set it to the beginning of the file name
			//otherwise set the beginning of the file name to empty IP address 0.0.0.0
			if (have_ip_address)
			{
				char ip_string[INET_ADDRSTRLEN];
				inet_ntop (AF_INET, &(from_addr.sin_addr), ip_string, sizeof ip_string);
				strcat(file_name, ip_string);
			}
			else 
			{
				char ip_string[INET_ADDRSTRLEN] = "0.0.0.0";
				strcat(file_name, ip_string);
			}	  

			//Get the current time
			time_t file_time = time(NULL);

			//Create a structure to convert time to human readable time
			struct tm file_time_tm;
			localtime_r(&file_time, &file_time_tm);

			//Create human readable time stamp
			char time_string[22];
			strftime (time_string, 21, "_%Y-%m-%d_%H:%M:%S", &file_time_tm);

			//Append time stamp to file name
			strcat(file_name, time_string);

			//Append file extension(i.e. '.raw') to file name
			strcat(file_name, extension);

			if(VERBOSE) printf("Saving to file: %s\n", file_name);


			//Open Output File
			FILE *file;
			file = fopen(file_name,"a");
			if (file == NULL)
			{
				fprintf(stderr, "ERROR #%d: File Open Failed", FILE_ERROR);
				close (recv_socket);
				return FILE_ERROR;
			}

			//Write to output to file
			if (fwrite((void*) buffer, 1, buf_len, file) != buf_len)
			{
				fprintf(stderr, "ERROR #%d: File Write Failed", FWRITE_ERROR);
				close (recv_socket);
				return FWRITE_ERROR;
			}

			//Close output file
			fclose(file);

			//Close Socket
			close (recv_socket);

			//End program and return success
			return SUCCESS;

		}
		else if (diff(lastWrite, currentTime).tv_sec >=  inter_experiment_sleep_time) 
		{

			//increase the inter_experiment_sleep_time so that it is much longer than experiment run time
			//because we dont want this if statment to be hit again
			inter_experiment_sleep_time = experiment_run_time + experiment_run_time;
			//Write the delimiter to the buffer and increment buffer by size of delim
			memcpy ((void*) (buffer + buf_len), (void*) delim, delim_size);
			buf_len += delim_size;

		}   

    }

}

//TODO: move them to the top ... global variable should not be initialize at declaration
char send_buffer[MAX_SEND_BUFFER_SIZE]; //Hardcoded based on max number of probe packets in 

int main(int argc, char *argv[])
{

  if(argc != 4){
    fprintf(stderr,"ERROR #%d: INVALID NUMBER OF ARGUMENTS\n", INVALID_NUMBER_OF_ARGUMENTS);
    fprintf(stderr, "Usage: ./receiver experiment_run_time probe_packet_length inter_experiment_sleep_time\n"); 
    return INVALID_NUMBER_OF_ARGUMENTS;
  }
  
  unsigned long initial_experiment_run_time = atoi(argv[1]);

  int probe_packet_length = atoi(argv[2]);

  unsigned long inter_experiment_sleep_time = atoi(argv[3]);

  //int                num_of_packets;  //TODO should be an argument?

  unsigned long later_experiment_run_time = initial_experiment_run_time/2;

  int i;
  for(i=0; i<MAX_SEND_BUFFER_SIZE;i++)
  {
    send_buffer[i] = 0;
  }

  if(UDPTrainReceiver(send_buffer, probe_packet_length, initial_experiment_run_time,  
  	later_experiment_run_time, inter_experiment_sleep_time)!= 0)
  {
    fprintf(stderr, "ERROR #%d: UDP Train Receiver Error", UDP_TRAIN_RECEIVER_FAILED);
    return UDP_TRAIN_RECEIVER_FAILED;                    
  }
  return 0;
}


