#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs.h"


char commandCount[64] = "count";

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


	//
	// We need a two way communication
	// For that, before forking, we need
	// to create two pipes
	// 
	// We will use one for getting input
	// the other one for sending output
	//
	
	// file descriptors for two way
	// communication
	int fd1[2];
	int fd2[2];


	// We create our pipes for two way
	// communication
	if(pipe(fd1) < 0 || pipe(fd2) < 0) {
		printf("There was an error while creating pipes!\n");
	}


	// Create child processes with fork

	// initial fork
	n = fork();

	// We only create another fork if
	// we are on parent process
	// we do not fork child processes
	for(int i=1; i<processCount; i++) {
		if(n > 0)
			n = fork();
	}

	// Parent process
	if(n!=0) {

		// fd1 is responsible for sending data
		// to child processes.
		//
		// So, we need to close read end of fd1
		// in parent process
		close(fd1[0]);

		// fd2 is responsible for getting data
		// from child processes.
		//
		// So, we need to close write end of fd2
		// in parent process
		close(fd2[1]);


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

		// This is the message which is going to be sent
		// to the child processes
		struct parent2Child messageFromParent;

		// This is the message being sent from child
		// processes
		struct child2Parent messageFromChild;

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


			//  --------- Broadcast command to all child processes -------- //
			for(int i=0; i<processCount; i++) {
				messageFromParent.inputPart   = i;					
				messageFromParent.argCount    = message.argCount;
				messageFromParent.commandType = message.commandType;
				for(int j=0; j<messageFromParent.argCount; j++)
					messageFromParent.argumentArray[j] = message.argumentArray[j];

				write(fd1[1], &messageFromParent, sizeof(struct parent2Child));
			}
			// -------------------------------------------- //

			// These commands are going to be passed
			// to child processes
			if(message.commandType == COUNT) {
				int count = 0;

				// Get responses for count command
				for(int i=0; i<processCount; i++) {
					read(fd2[0], &messageFromChild, sizeof(struct child2Parent));
					if(messageFromChild.responseFlag != -1) {
						perror("something went weird!\n");
						exit(1);
					}
					count += messageFromChild.responseVal;					
				}

				messageResponse.responseVal = count;
			}
			else if(message.commandType == AVG) {
				float sum = 0;
				int count = 0;

				// Get responses for avg command
				for(int i=0; i<processCount; i++) {
					read(fd2[0], &messageFromChild, sizeof(struct child2Parent));
					if(messageFromChild.responseFlag != -1) {
						perror("something went weird!\n");
						exit(1);
					}

					sum += messageFromChild.responseVal;
					count += messageFromChild.valCountForAvg;
				}

				float avg = sum / count;

				messageResponse.responseVal = avg;
			}
			else if(message.commandType == MAX) {
				float max = 0;

				// Get responses for avg command
				for(int i=0; i<processCount; i++) {
					read(fd2[0], &messageFromChild, sizeof(struct child2Parent));
					if(messageFromChild.responseFlag != -1) {
						perror("something went weird!\n");
						exit(1);
					}
					if(messageFromChild.responseVal >= max)
						max = messageFromChild.responseVal;
				}


				messageResponse.responseVal = max;

			}
			else if(message.commandType == RANGE) {
				int count = 0;

				// Get responses for avg command
				for(int i=0; i<processCount; i++) {
					read(fd2[0], &messageFromChild, sizeof(struct child2Parent));
					if(messageFromChild.responseFlag != -1) {
						perror("something went weird!\n");
						exit(1);
					}
					for(int j=count; j<count + messageFromChild.rangeItemCount; j++) {
						messageResponse.rangeResp[j] = messageFromChild.rangeResp[j - count];
					}
					count += messageFromChild.rangeItemCount;
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
		return 0;
	}
	// Child processes
	else if(n == 0) {

		fflush(stdout);		
		// fd1 is responsible for sending data
		// to child processes.
		//
		// So, we need to close write end of fd1
		// in child process
		close(fd1[1]);

		// fd2 is responsible for getting data
		// from child processes.
		//
		// So, we need to close read end of fd2
		// in child process
		close(fd2[0]);

		// Dummy data
		char data[16];

		// This is the message which is going to be sent
		// to the child processes
		struct parent2Child messageFromParent;

		// This is the message being sent from child
		// processes
		struct child2Parent messageFromChild;		

		while(1) {

			// Read message
			read(fd1[0], &messageFromParent, sizeof(struct parent2Child));

			// process commands
			if(messageFromParent.commandType == COUNT) {

				// if we don't have arguments, just take the count
				if(messageFromParent.argCount == 0) {
					int count = inputArray[messageFromParent.inputPart].count;
					messageFromChild.responseFlag = -1;
					messageFromChild.responseVal  = count;
				}
				else if(messageFromParent.argCount == 2) {
					int start = messageFromParent.argumentArray[0];
					int end   = messageFromParent.argumentArray[1];
					int count = 0;

					for(int i=0; i<inputArray[messageFromParent.inputPart].count; i++) {
						if(inputArray[messageFromParent.inputPart].data[i] >= start &&
						   inputArray[messageFromParent.inputPart].data[i] <= end)
							count++;
					}

					messageFromChild.responseFlag = -1;
					messageFromChild.responseVal  = count;
				}
				// There is an argument error
				else {
					messageFromChild.responseFlag = 0;
				}
			}
			else if(messageFromParent.commandType == AVG) {
				float sum = 0;
				
				// if we don't have any arguments
				if(messageFromParent.argCount == 0) {
					
					for(int i=0; i<inputArray[messageFromParent.inputPart].count; i++) {
						sum += inputArray[messageFromParent.inputPart].data[i];
					}
					messageFromChild.responseFlag = -1;					
					messageFromChild.responseVal = sum;
					messageFromChild.valCountForAvg = inputArray[messageFromParent.inputPart].count;

				}
				else if(messageFromParent.argCount == 2) {
					int start = messageFromParent.argumentArray[0];
					int end   = messageFromParent.argumentArray[1];					
					int count = 0;

					for(int i=0; i<inputArray[messageFromParent.inputPart].count; i++) {
						if(inputArray[messageFromParent.inputPart].data[i] >= start &&
						   inputArray[messageFromParent.inputPart].data[i] <= end) {
							count++;
							sum += inputArray[messageFromParent.inputPart].data[i];							
						}
					}
					messageFromChild.responseFlag = -1;					
					messageFromChild.responseVal = sum;
					messageFromChild.valCountForAvg = count;					
				}
				// There is an argument error
				else {
					messageFromChild.responseFlag = 0;
				}
			}
			else if(messageFromParent.commandType == MAX) {

				// Arrays are already sorted, so we just
				// take the last element of our array
				int arraySize = inputArray[messageFromParent.inputPart].count;
				messageFromChild.responseFlag = -1;
				messageFromChild.responseVal  = inputArray[messageFromParent.inputPart].data[arraySize-1];
			}
			else if(messageFromParent.commandType == RANGE) {
				int start = messageFromParent.argumentArray[0];
				int end   = messageFromParent.argumentArray[1];
				int k     = messageFromParent.argumentArray[2];
				int count = 0;

				for(int i=0; i<inputArray[messageFromParent.inputPart].count; i++) {
					if(inputArray[messageFromParent.inputPart].data[i] >= start &&
						inputArray[messageFromParent.inputPart].data[i] <= end) {
						messageFromChild.rangeResp[count] = inputArray[messageFromParent.inputPart].data[i];							
						if(count == k)
							break;
						count++;
					}
				}

				messageFromChild.responseFlag = -1;
				messageFromChild.rangeItemCount = count;

			}

			// Send calculated child response to the parent
			write(fd2[1], &messageFromChild, sizeof(struct child2Parent));

		}

		exit(0);
	}


   	for (int i=0; i<processCount; ++i)
       	wait(NULL); 

   	printf ("all children terminated. bye... \n");	

}
