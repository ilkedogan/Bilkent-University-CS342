#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs_th.h"

#define MAX_THREAD;


char commandCount[64] = "count";

struct lastCommand G_LAST_COMMAND;

// ---------------------- THREAD FUNCTIONS ----------------- //

static void *do_task(void *arg_ptr) {
	

	struct threadState* s = (struct  threadState*) arg_ptr;

	while(1) {
		if(s->state == RUNNING) {		

			if(G_LAST_COMMAND.commandType == COUNT) {

				// if we don't have arguments, just take the count
				if(G_LAST_COMMAND.argCount == 0) {
					int count = s->inputPart->count;
					s->responseFlag = -1;
					s->responseVal  = count;
				}
				else if(G_LAST_COMMAND.argCount == 2) {
					int start = G_LAST_COMMAND.argumentArray[0];
					int end   = G_LAST_COMMAND.argumentArray[1];
					int count = 0;

					for(int i=0; i<s->inputPart->count; i++) {
						if(s->inputPart->data[i] >= start &&
						   s->inputPart->data[i] <= end)
							count++;
					}

					s->responseFlag = -1;
					s->responseVal  = count;
				}
				// There is an argument error
				else {
					s->responseFlag = 0;
				}
			}
			else if(G_LAST_COMMAND.commandType == AVG) {
				float sum = 0;
				
				// if we don't have any arguments
				if(G_LAST_COMMAND.argCount == 0) {

					for(int i=0; i<s->inputPart->count; i++) {
						sum += s->inputPart->data[i];
					}
						
					s->responseFlag = -1;					
					s->responseVal = sum;
					s->valCountForAvg = s->inputPart->count;

				}
				else if(G_LAST_COMMAND.argCount == 2) {
					int start = G_LAST_COMMAND.argumentArray[0];
					int end   = G_LAST_COMMAND.argumentArray[1];					
					int count = 0;

					for(int i=0; i<s->inputPart->count; i++) {
						if(s->inputPart->data[i] >= start &&
						   s->inputPart->data[i] <= end) {
							count++;
							sum += s->inputPart->data[i];							
						}
					}
					s->responseFlag = -1;					
					s->responseVal = sum;
					s->valCountForAvg = count;					
				}
				// There is an argument error
				else {
					s->responseFlag = 0;
				}
			}
			else if(G_LAST_COMMAND.commandType == MAX) {

				// Arrays are already sorted, so we just
				// take the last element of our array
				int arraySize = s->inputPart->count;
				s->responseFlag = -1;
				s->responseVal  = s->inputPart->data[arraySize-1];
			}
			else if(G_LAST_COMMAND.commandType == RANGE) {
				int start = G_LAST_COMMAND.argumentArray[0];
				int end   = G_LAST_COMMAND.argumentArray[1];
				int k     = G_LAST_COMMAND.argumentArray[2];
				int count = 0;

				for(int i=0; i<s->inputPart->count; i++) {
					if(s->inputPart->data[i] >= start &&
						s->inputPart->data[i] <= end) {
						s->rangeResp[count] = s->inputPart->data[i];							
						if(count == k)
							break;
						count++;
					}
				}

				s->responseFlag = -1;
				s->rangeItemCount = count;

			}
		}

		s->state = WAITING;
	
	}
}


// ----------------------------- UTILITY FUNCTIONS --------------------- //
void swap(int *p1, int *p2) {
	int tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

// we perform selection sort
void sortArray(int* array, int size) {
	int min_idx;

	for(int i=0; i<size-1; i++) {

		min_idx = i;
		for(int j=i+1; j<size; j++)
			if(array[j] < array[min_idx])
				min_idx = j;

		swap(&array[min_idx], &array[i]);
	}
}

// Reads file inputs
struct input readFile(char *inputFileName) {

	FILE *file;
	char line[512];
	struct input result;

	file = fopen(inputFileName, "r");

	if (!file)
		exit(1);

	int count = 0;

	// First we get how many integers we need
	// We assume that in the input file,
	// size of array is not determined
	while(fgets(line, sizeof(line), file))
		count++;

	// write count to result
	result.count = count;

	// Return to the beginning of the file
	rewind(file);

	// Allocate memory for input data using count value
	result.data = (int*) malloc(sizeof(int)*count);
	int dataCounter = 0;

	// Read every value one by one and write it
	// to the allocated memory of result struct
	while(fgets(line, sizeof(line), file)) {
		int val = atoi(line);
		result.data[dataCounter] = val;
		dataCounter++;
	}

	fclose(file);
	return result;
}
// --------------------------------------------------------------- //


/**
 * ----------------- Server Problem ------------------
 * 
 *  Server may have to process N different
 *  input files.
 * 
 *  We need different child processes for
 *  every input file. These processes will
 *  run commands concurrently and send 
 *  results to the parent process. Then
 *  parent process merges all results
 *  and sends final result to client
 * 
 *  We get commands from the client via
 *  message queue.
 * 
 */
int main(int argv, char** argc)
{

	pthread_t tids[10];


	// Get how many process we need from arguments
	int processCount = atoi(argc[1]);

	// Allocate memory for input array. Each child process
	// will use one of the inputs
	struct input *inputArray = (struct input*) malloc(sizeof(struct input)* processCount);
	
	// Fill in input arrays with integers read
	// from input files
	for(int i=0; i<processCount;i++) {
		inputArray[i] = readFile(argc[2 + i]);

		// Sort array rightaway
		sortArray(inputArray[i].data, inputArray[i].count);
	}

	// This variable will store
	// process id
	pid_t n;


	int ret;

	struct threadState* threadStates = (struct threadState*) malloc(sizeof(struct threadState) * processCount);
	
	for(int i=0; i<processCount; i++) {
		threadStates[i].state = WAITING;
		threadStates[i].inputPart = &inputArray[i];
	}


	for(int i=0; i<processCount; i++) {

		ret = pthread_create(&(tids[i]),
			NULL, do_task, (void *) &(threadStates[i]));

		if(ret != 0) {
			printf("thread create failed \n");
			exit(1);			
		}

		printf("thread %i with tid %u created\n", i,
		       (unsigned int) tids[i]);		

		
	}


	// Parent process takes commands from
	// client via message que
	//
	// the following code is for handling
	// client requests

	// Send and receive response values
	// we will use them to check whether
	// we have sent or received successfully
	int receiveResp, sendResp;

	// Message queues configuration vars
	// for receiver and sender
	mqd_t mqReceiver, mqSender;
	struct mq_attr mq_attr;

	// Receiver msg queue has been opened
	mqReceiver = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
	if(mqReceiver == -1) {
		perror("cannot create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqReceiver);
	mq_getattr(mqReceiver, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

	// Sender msg queue has been opened
	mqSender = mq_open(MQNAME, O_RDWR);
	if (mqSender == -1) {
		perror("can not open msg queue\n");
		exit(1);
	}
	printf("mq opened, mq id = %d\n", (int) mqSender);


	// This message struct below is going to be filled
	// when data taken from client
	struct item message;

	// This message struct below is going to be filled
	// when response of server is sent back to client
	struct itemResponse messageResponse;

	// Main loop of parent process of our server function
	while(1) {		

		receiveResp = mq_receive(mqReceiver, (char *) &message, mq_attr.mq_msgsize, NULL);
		if (receiveResp == -1) {
			perror("mq_receive failed\n");
			printf("%d\n", sizeof(message));
			exit(1);
		}
		
		//printf("Received Command -> id = %d\n", message.commandType);
		//printf("Received argument count -> argCount = %d\n", message.argCount);


		G_LAST_COMMAND.argCount = message.argCount;
		G_LAST_COMMAND.commandType = message.commandType;
		for(int j=0; j<G_LAST_COMMAND.argCount; j++)
			G_LAST_COMMAND.argumentArray[j] = message.argumentArray[j];		

		//  --------- Broadcast command to all child processes -------- //
		for(int i=0; i<processCount; i++) {
			threadStates[i].state = RUNNING;
			threadStates[i].inputPart = &inputArray[i];
		}
		// -------------------------------------------- //


		while(1) {			
			int readyFlag = 1;
			for(int i=0; i<processCount; i++) {
				if(threadStates[i].state == RUNNING)
					readyFlag = 0;
			}

			if(readyFlag) {
				break;
			}

		}	

		// These commands are going to be passed
		// to child processes
		if(message.commandType == COUNT) {
			int count = 0;
			
			// Get responses for count command
			for(int i=0; i<processCount; i++) {
				count += threadStates[i].responseVal;					
			}

			messageResponse.responseVal = count;
		}
		else if(message.commandType == AVG) {
			float sum = 0;
			int count = 0;

			// Get responses for avg command
			for(int i=0; i<processCount; i++) {
				sum += threadStates[i].responseVal;
				count += threadStates[i].valCountForAvg;
			}

			float avg = sum / count;

			messageResponse.responseVal = avg;
		}
		else if(message.commandType == MAX) {
			float max = 0;

			// Get responses for avg command
			for(int i=0; i<processCount; i++) {
				if(threadStates[i].responseVal >= max)
					max = threadStates[i].responseVal;
			}

			messageResponse.responseVal = max;

		}
		else if(message.commandType == RANGE) {
			int count = 0;

			// Get responses for avg command
			for(int i=0; i<processCount; i++) {
				for(int j=count; j<count + threadStates[i].rangeItemCount; j++) {
					messageResponse.rangeResp[j] = threadStates[i].rangeResp[j - count];
				}
				count += threadStates[i].rangeItemCount;
			}

			// we need to sort merged array again
			sortArray(messageResponse.rangeResp, count);

			messageResponse.rangeItemCount = count;

		}


		sendResp = mq_send(mqSender, (char *) &messageResponse, mq_attr.mq_msgsize, 0);
		if (sendResp == -1) {
			perror("mq_send failed\n");
			exit(1);
		}
		sleep(1);

	}
	
	mq_close(mqReceiver);
	mq_close(mqSender);


	printf("main: waiting all threads to terminate\n");
	for (int i = 0; i < processCount; ++i) {
		ret = pthread_join(tids[i], NULL);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(0);
		}
	}


	printf("main: all threads terminated\n");

}
