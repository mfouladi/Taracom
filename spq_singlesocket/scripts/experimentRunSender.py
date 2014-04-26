#!/usr/bin/python

#
# Python script to run both low and high entropy and
# write the data results into a file at the sender
#
# How to run this script:
# python ~/triton/experimentRunsender.py experiment_config_file_name
#

import subprocess
import sys
import datetime
import time
import ConfigParser

#Open Config File to Extract Values
experiment_config_file_name = sys.argv[1]
config = ConfigParser.RawConfigParser()
config.readfp(open(experiment_config_file_name))

#Read Experiment Run Values from Config File
initial_num_of_packets = config.getint('DEFAULT', 'initial_num_of_packets') #receive from config file
seperation_train_length = config.getint('DEFAULT', 'seperation_train_length') #receive from config file
num_packet_trains = config.getint('DEFAULT', 'num_packet_trains') #receive from config file
probe_packet_length = config.getint('DEFAULT', 'probe_packet_length') #receive from config file
compression_node_addr = config.get('DEFAULT', 'compression_node_addr') #receive from config file

#udp_session_timeout = config.getint('DEFAULT', 'udp_session_timeout') #receive from config file
#inter_packet_departure_spacing = config.getint('DEFAULT', 'inter_packet_departure_spacing') #receive from config file
#dest_addr = config.get('DEFAULT', 'dest_addr') #receive from config file

log_file_path = config.get('DEFAULT', 'log_file_path') #receive from config file
#results_data_file_path = config.get('DEFAULT', 'results_data_file_path') #receive from config file
inter_experiment_sleep_time = config.getint('DEFAULT', 'inter_experiment_sleep_time') #receive from config file
experiment_scenario_id = config.getint('DEFAULT', 'experiment_scenario_id') #receive from config file

#set time stamp
current_time = datetime.datetime.now() #formatted time from python
current_timestamp_string = current_time.strftime("%Y-%m-%d--%H-%M")

# Run for low entropy
entropy = 'H'
#results_data_file = results_data_file_path + current_timestamp_string + str(experiment_scenario_id) + '_L.dat'
log_data_file = log_file_path + current_timestamp_string + str(experiment_scenario_id) + '_L.log'

#open log file to write to
log_file = open(log_data_file, 'w+')

#arguments to be given to subprocess
args = ["./unitExperimentSender",initial_num_of_packets, seperation_train_length, num_packet_trains, probe_packet_length, compression_node_addr, entropy]
str_args = [ str(x) for x in args ] #convert args to string

# Execute the following command in terminal
#./unitExperimentSender num_of_packets inter_packet_departure_spacing probe_packet_length "results_data_file" "dest_addr" "compression_node_addr" timeout 'L' > log_data_file
runExperiment = subprocess.Popen(str_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout_value, stderr_value = runExperiment.communicate()
log_file.write(repr(stdout_value))
log_file.write(repr(stderr_value))
log_file.write(str(runExperiment.returncode))
log_file.close()


# Wait 1 minute after first unitExperiment is done
time.sleep(inter_experiment_sleep_time)

#reset time stamp
#current_time = datetime.datetime.now() #formatted time from python
#current_timestamp_string = current_time.strftime("%Y-%m-%d::%H:%M")

# Run for high entropy
entropy = 'L'
#results_data_file = results_data_file_path + current_timestamp_string+ str(experiment_scenario_id) + '_H.dat'
log_data_file = log_file_path + current_timestamp_string + str(experiment_scenario_id) + '_H.log'

#open log file to write to
log_file = open(log_data_file, 'w+')

#arguments to be given to subprocess
args = ["./unitExperimentSender",initial_num_of_packets, seperation_train_length, num_packet_trains, probe_packet_length, compression_node_addr, entropy]
str_args = [ str(x) for x in args ] #convert args to string

# Execute the following command in terminal
#./unitExperimentSender num_of_packets inter_packet_departure_spacing probe_packet_length "results_data_file" "dest_addr" "compression_node_addr" timeout 'H' > log_data_file
runExperiment = subprocess.Popen(str_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout_value, stderr_value = runExperiment.communicate('through stdin to stdout')
log_file.write(repr(stdout_value))
log_file.write(repr(stderr_value))
log_file.write(str(runExperiment.returncode))
log_file.close()


meta_data_file = current_timestamp_string + str(experiment_scenario_id) +'.meta'

