// Enumeration for command types
enum COMMAND_TYPES {
	COUNT = 0,
	AVG   = 1,
	MAX   = 2,
	RANGE = 3
};


// This struct keeps data of
// input files
struct input {
	size_t count;
	int* data;
};

// This is the struct of messages
// that are going to be sent to
// the server
struct item {

	// These two are going to be deleted soon :D
	int id;		
	char astr[64];

	// Command types are integers
	int commandType;
	int argCount;
	// We can have three arguments
	// at most. They are;
	//   1. start
	//   2. end
	//   3. k
	int argumentArray[3];
	
};


// This is the struct of messages
// that are going to be sent to client
// from server as response
struct itemResponse {

	// response of avg, count
	// and max commands
	float responseVal;

	// It is said we can at most
	// have 1000 integers in range
	// response
	int rangeItemCount;	
	int rangeResp[3000];
};

// This struct is for messages sent
// to child processes
struct parent2Child {

	// This determines which part
	// of input child going to process
	int inputPart;

	int commandType;
	int argCount;

	// We can have three arguments
	// at most. They are;
	//   1. start
	//   2. end
	//   3. k
	int argumentArray[3];	
};

// This struct is for message sent
// to the parent process
struct child2Parent {

	// We determine calculations
	// successfully finished
	int responseFlag;

	// response of avg, count
	// and max
	float responseVal;

	// for avg, we need how many
	// vals in that range
	int valCountForAvg;

	// It is said we can at most
	// have 1000 integers in range
	// response
	int rangeItemCount;
	int rangeResp[1000];

};


#define MQNAME "/justaname"
