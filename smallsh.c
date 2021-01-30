#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct userInput {
	char* command;
	char** arguments;
	int argCount;
	int redirectIn;
	int redirectOut;
	char* inputFile;
	char* outputFile;
	int isBackground;
};

struct userInput* parseInput(char* inputLine);
void freeInputStruct(struct userInput* inputStruct);
void checkSpecialSymbols(struct userInput* inputStruct);

int main(int argc, const char* argv[]) {
	// create a command line
	char* input = NULL;
	char* exit = "exit";
	size_t inputLength; 
	//ssize_t nread;
	struct userInput* userInput = NULL;
	do {
		if (userInput) { 
			freeInputStruct(userInput); 
		};
		printf(": ");
		getline(&input, &inputLength, stdin);
		userInput = parseInput(input);
	} while (strncmp(userInput->command, exit, 4) != 0);

	// clean input struct
	//free(input);
	free(input);
	freeInputStruct(userInput);

}

// receives the input line and parses the command
// and arguments
struct userInput* parseInput(char* inputLine)
{
	// init the struct
	struct userInput* parsedInput = malloc(sizeof(struct userInput));
	// use strtok to parse the input line
	char* savePtr;
	char* token = strtok_r(inputLine, " ", &savePtr);
	// first input is the command
	parsedInput->command = strdup(token);

	// loop to store the remaining arguments
	parsedInput->arguments = calloc(513, sizeof(char*)); //create array of char ptrs
	int i = 0;
	while ((token = strtok_r(NULL, " ", &savePtr)) != NULL) {
		// store each arg
		parsedInput->arguments[i] = strdup(token);
		i++;
	}
	// note the number of arguments
	parsedInput->argCount = i;
	// check for special chars
	checkSpecialSymbols(parsedInput);

	return parsedInput;
}


void checkSpecialSymbols(struct userInput* inputStruct) {
	char* amp = "&";
	char* greater = ">";
	char* lessThan = "<";
	// check if command should redirect and get filenames

	// check if command should run in background
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
