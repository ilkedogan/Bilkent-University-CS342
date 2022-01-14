#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>       
#include "shareddefs_th.h"

/**
 * ----------------------- Client Problem -----------------
 * 
 *  Client takes user input and sends it to the server.
 * 
 * 	Server processes these inputs with multiple processes
 *  and returns a response. This response might be a single
 *  integer value, float value or an integer array
 * 
 *  We have to parse user input and form message to the server
 *  we will fill in command type and arguments if we have any
 * 
 */
int main()
{
	clock_t start_time = clock();
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

	// This message struct below is going to be sent
	// to the server
	struct item message;

	// This message struct below is going to be filled
	// when response of server is sent back to client
	struct itemResponse messageResponse;	

	// This char array is for taking input strings
	char inputStr[256];

	while(1) {


		printf("Enter the request code: ");
		fgets(inputStr, sizeof(inputStr), stdin);

		// We need to parse inputStr and extract our command and arguments
		// We first extract our command
		char *token = strtok(inputStr, " ");

		// keeps track of where we are parsing
		// the user input
		int commandParseCounter = 0;

		// initialize argument count as 0
		message.argCount = 0;

		// Now extract arguments if there are any
		while(token != NULL) {

			// We are reading the command
			if(commandParseCounter == 0) {
				if(strncmp(token,"count", strlen("count")) == 0)
					message.commandType = COUNT;
				else if(strncmp(token, "avg", strlen("avg")) == 0)
					message.commandType = AVG;
				else if(strncmp(token, "max", strlen("max")) == 0)
					message.commandType = MAX;
				else if(strncmp(token, "range", strlen("range")) == 0)
					message.commandType = RANGE;
			}
			else {
				message.argCount = commandParseCounter;
				message.argumentArray[commandParseCounter - 1] = atoi(token);
			}

			token = strtok(NULL, " ");

			commandParseCounter++;
		}

		sendResp = mq_send(mqSender, (char *) &message, sizeof(struct item), 0);

		if (sendResp == -1) {
			perror("mq_send failed\n");
			exit(1);
		}

		printf("---------------------------------------\n");
		printf("Sent message -> commandType = %d\n", message.commandType);
		//sleep(1);	

		receiveResp = mq_receive(mqReceiver, (char *) &messageResponse, mq_attr.mq_msgsize, NULL);
		if (receiveResp == -1) {
			perror("mq_receive failed\n");
			exit(1);
		}

		// If command type is range, we have
		// to return integer array
		if(message.commandType == RANGE) {

			printf("---------------------------------------\n");
			printf("Received integer array with size %d:\n", messageResponse.rangeItemCount);

			for(int i=0; i<messageResponse.rangeItemCount; i++)
			{
				printf("%d ", messageResponse.rangeResp[i]);
			}
			printf("\n");
			printf("---------------------------------------\n");
		}
		else
		{
			printf("---------------------------------------\n");			
			printf("Received response val -> id = %f\n", messageResponse.responseVal);
			printf("---------------------------------------\n");			
		}

		clock_t end_time = clock();
		printf("The program working time: %zd milliseconds.\n", end_time - start_time);
	}

	mq_close(mqSender);
	mq_close(mqReceiver);
	return 0;
}

