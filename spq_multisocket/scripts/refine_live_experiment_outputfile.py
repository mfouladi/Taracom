# David Meyer, 4/3/14
#
# INPUT: A raw experiment output file with two columns, and the number
# of packets that the output file is expected to have.
#
# Example raw outputfile:
# Column 1: Packet number       Column 2: Packet timestamp
#   0                           12.287327749
#   1                           -1
#   2                           12.287540652
#  etc.
# 
# A value of -1 in Column 2 means that the packet was lost/dropped.
# Some output files may have an asterisk in column 1 and nothing in
# column 2. This * separates two larger chunks of packet data. We want
# to break the output file into two subfiles, using the asterisk as
# the breakpoint.
#
# If the original outputfile contains less than "number_of_packets"
# packets, then we need to fill out the rest of the outputfile with
# -1's, to account for those packets that were lost.
#
# OUTPUT: Two experiment output files, broken apart at the asterisk.
# originalname.raw --> originalname_A.dat & originalname_B.dat

# If there is no asterisk in the input file, then we'll only produce
# the _A.dat file.
# originalname.raw --> originalname_A.dat
#
# Example:  (VAHAB)
# refine_live_experiment_outputfile("131.179.192.201_2014-03-25_17_53_47.raw", 10100)
#


import os

def refine_live_experiment_outputfile(filename, number_of_packets, refined_results_file_path):
    # break raw experiment file into two subfiles at the asterisk
    input = open(filename, 'rU')
    data = input.readlines()
    input.close()
    A = []
    B = []
    # Extract the pure filename (no extension) for writing purposes
    fname = os.path.splitext(filename)[0]
    outputA = open(refined_results_file_path+fname + "_A.dat", 'w') # Open the "_A.dat" file

    ###################################
    # If asterisk, split into two files
    ###################################
    splitchar = "*\n"
    # If the file contains an asterisk, split into A and B lists
    if splitchar in data:
        outputB = open(fname + "_B.dat", 'w') # We'll need to also open a "_B.dat" file
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


refine_live_experiment_outputfile(temp_file, num_of_packets, refined_results_file_path)