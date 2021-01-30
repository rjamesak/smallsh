#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct userInput {
	char* command;
	char** arguments;
};

struct userInput* parseInput(char* inputLine, int inputLength);
void freeInputStruct(struct userInput* inputStruct);

int main(int argc, const char* argv[]) {
	// create a command line
	char* input = NULL;
	char* exit = "exit";
	size_t inputLength; 
	ssize_t nread;
	struct userInput* userInput = NULL;
	do {
		if (userInput) { 
			freeInputStruct(userInput); 
		};
		printf(": ");
		nread = getline(&input, &inputLength, stdin);
		userInput = parseInput(input, (int) nread);
	} while (strncmp(userInput->command, exit, 4) != 0);

	// clean input struct
	//free(input);
	free(input);
	freeInputStruct(userInput);

}

// receives the input line and parses the command
// and arguments
struct userInput* parseInput(char* inputLine, int inputLength)
{
	// init the struct
	struct userInput* parsedInput = malloc(sizeof(struct userInput));
	// use strtok to parse the input line
	char* savePtr;
	char* token = strtok_r(inputLine, " ", &savePtr);
	// first input is the command
	// NOTE strdup allocs mem, remember to free
	parsedInput->command = strdup(token);

	// loop to store the remaining arguments
	parsedInput->arguments = calloc(inputLength, sizeof(char*)); //create array of char ptrs
	int i = 0;
	while ((token = strtok_r(NULL, " ", &savePtr)) != NULL) {
		// store each arg
		parsedInput->arguments[i] = strdup(token);
		i++;
	}
	// terminate the arg array
	//parsedInput->arguments[inputLength - 1] = '\0';

	return parsedInput;
}

void freeInputStruct(struct userInput* inputStruct)
{
	// free the command
	free(inputStruct->command);
	// loop the arg array and free each
	int i = 0;
	while (inputStruct->arguments[i] != '\0') {
		free(inputStruct->arguments[i]);
		i++;
	}
	free(inputStruct->arguments);
	free(inputStruct);
}
