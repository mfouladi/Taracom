
CC			=	gcc
IDIR		=	include
SENDDIR 	=   sender
RECVDIR 	=	receiver
CFLAGS		=	-O3 -Wall -fno-strict-aliasing -I $(IDIR)

HDR			=	unitExperiment.h
SENDSRC		=	$(SENDDIR)/UDPTrainGenerator.c
RECVSRC		=	$(RECVDIR)/UDPTrainReceiver.c
SENDOBJ 	=	unitExperimentSender
RECVOBJ 	=	unitExperimentReceiver

all: unitExperimentSender unitExperimentReceiver

%.o: %.c HDR
	$(CC) -c -o $@ $< $(CFLAGS)

#-lrt is used for system clock function get_clock_time
#gcc -O3 -Wall -fno-strict-aliasing -I ./include ./sender/*.c -lrt -o unitExperimentSender
unitExperimentSender: $(SENDSRC)
	$(CC) $(CFLAGS) $(SENDSRC) -lrt -o $(SENDOBJ)

#-lrt is used for system clock function get_clock_time
#gcc -O3 -Wall -fno-strict-aliasing -I ./include ./receiver/*.c -lrt -o unitExperimentReceiver
unitExperimentReceiver: $(RECVSRC)
	$(CC) $(CFLAGS) $(RECVSRC) -lrt -o $(RECVOBJ)

.PHONY:	clean

clean:
	rm $(SENDOBJ)
	rm $(RECVOBJ)

