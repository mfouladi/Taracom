#!/usr/bin/python

#
# Python script to run both receiver.c with given
# arguments and fix the output of the file so all
# packets are accounted for in the .raw file
#
# How to run this script:
# python ~/triton/experimentRunReceiver.py experiment_config_file_name
#

import os
import subprocess
import sys
import datetime
import time
import ConfigParser

def refine_live_experiment_outputfile(filename, number_of_packets, refined_results_file_path):
    # break raw experiment file into two subfiles at the asterisk
    input = open(filename, 'r')
    data = input.readlines()
    input.close()
    A = []
    B = []
    # Extract the pure filename (no extension) for writing purposes
    fname = os.path.splitext(filename)[0]
    fname = fname.split('/')[1]
    outputA = open(refined_results_file_path+fname + "_L.dat", 'w') # Open the "_A.dat" file

    ###################################
    # If asterisk, split into two files
    ###################################
    splitchar = "*\n"
    # If the file contains an asterisk, split into A and B lists
    if splitchar in data:
        outputB = open(refined_results_file_path+fname + "_H.dat", 'w') # We'll need to also open a "_B.dat" file
        s_index = data.index(splitchar)
        # Go up until the asterisk...
        for i in range(0, s_index):
            A.append(data[i])
        # Start one past the asterisk, go until the end
        for i in range(s_index + 1, len(data)):
            B.append(data[i])
    else: # No asterisk => just the A list
        for i in range(0, len(data)):
            A.append(data[i])

    ###################################
    # Fill in with "-1" up to "number_of_packets"
    ###################################
    filelists = [A]
    if len(B): filelists.append(B) # Only add B to the list if it has data
    for x in filelists:
        for j in range(len(x), number_of_packets):
            # We will not go inside this loop if len(x) >= number_of_packets
            x.append(str(j) + "\t-1\n")

    if len(A):
        outputA.write("".join(A))
        # print "_A.dat was successful"

    if len(B):
        outputB.write("".join(B))
        # print "_B.dat was successful"


#Open Config File to Extract Values
experiment_config_file_name = sys.argv[1]
config = ConfigParser.RawConfigParser()
config_file = open(experiment_config_file_name)
config.readfp(config_file)

#Read Experiment Run Values from Config File
probe_packet_length = config.getint('DEFAULT', 'probe_packet_length') #receive from config file
udp_session_timeout = config.getint('DEFAULT', 'udp_session_timeout') #receive from config file
num_of_packets = config.getint('DEFAULT', 'num_of_packets') #receive from config file

log_file_path = config.get('DEFAULT', 'log_file_path') #receive from config file
raw_results_file_path = config.get('DEFAULT', 'raw_results_file_path') #receive from config file
refined_results_file_path = config.get('DEFAULT', 'refined_results_file_path') #receive from config file
temp_results_file_path = config.get('DEFAULT', 'temp_results_file_path') #receive from config file
experiment_scenario_id = config.getint('DEFAULT', 'experiment_scenario_id') #receive from config file

# Close config file
config_file.close()

#set time stamp
current_time = datetime.datetime.now() #formatted time from python
current_timestamp_string = current_time.strftime("%Y-%m-%d--%H-%M")

#open log file to write to
#log_data_file = log_file_path + current_timestamp_string + str(experiment_scenario_id) + '.log'
#log_file = open(log_data_file, 'w+')

#arguments to be given to subprocess
#args = ["./unitExperimentReceiver",probe_packet_length, udp_session_timeout]
#str_args = [ str(x) for x in args ] #convert args to string
args = "./unitExperimentReceiver " + str(udp_session_timeout)  + ' '  + str(probe_packet_length)

# Run receiver
# Execute the following command in terminal
#./unitExperimentReceiver probe_packet_length udp_session_timeout > log_data_file
os.system(args)

# runExperiment = subprocess.Popen(str_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# stdout_value, stderr_value = runExperiment.communicate()

# # # Write to Log File
# log_file.write(repr(stdout_value))
# log_file.write(repr(stderr_value))
# log_file.write(str(runExperiment.returncode))
# log_file.close()


#refine_live_experiment_outputfile("", 1, "./")

# #get raw file name
temp_files = os.listdir(temp_results_file_path)
if (len(temp_files) == 1):
	temp_file = temp_results_file_path + temp_files[0]

	# # Refine the raw file and store in new refined file
	refine_live_experiment_outputfile(temp_file, num_of_packets, refined_results_file_path)

	# Move raw file to raw file path
	raw_file = raw_results_file_path + temp_file.split('/')[1]
	os.rename(temp_file, raw_file)
	exit()
else:
	print "no"
	exit()