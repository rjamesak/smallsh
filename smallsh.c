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
void expandVariables(struct userInput* inputStruct);
void expandVarInCommand(struct userInput* inputStruct);

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
		// action based on input
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
	parsedInput->argCount = 0;
	parsedInput->isBackground = 0;
	parsedInput->redirectIn = 0;
	parsedInput->redirectOut = 0;
	
	// use strtok to parse the input line
	char* comment = "#";
	char* newline = "\n";
	char* savePtr;
	char* token = strtok_r(inputLine, " ", &savePtr);
	// first input is the command
	parsedInput->command = strdup(token);
	// check if comment
	//if (strcmp(token, comment) == 0 || strcmp(token, newline) == 0) {
	//	return parsedInput;
	//}

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
	expandVariables(parsedInput);
	expandVarInCommand(parsedInput);

	return parsedInput;
}


void checkSpecialSymbols(struct userInput* inputStruct) {
	char* amp = "&\n";
	int args = inputStruct->argCount;
	char* greater = ">\0";
	char* lessThan = "<\0";
	int redirectsIn = 0;
	int redirectIndexIn = 0;
	int redirectsOut = 0;
	int redirectIndexOut = 0;
	// check if command should redirect and get filenames
	for (int i = 0; i < args - 1; i++) {
		// input redirect
		if (strcmp(inputStruct->arguments[i], lessThan) == 0) {
			redirectIndexIn = i;
			inputStruct->redirectIn = 1;
			// copy file to input file
			inputStruct->inputFile = strdup(inputStruct->arguments[i+1]);
			redirectsIn = 2; // will reduce args by this number
			// null these from the arg array
			//inputStruct->arguments[i] = "\0";
			//inputStruct->arguments[i + 1] = "\0";
		}
		// output redirect
		if (strcmp(inputStruct->arguments[i], greater) == 0) {
			redirectIndexOut = i;
			inputStruct->redirectOut = 1;
			inputStruct->outputFile = strdup(inputStruct->arguments[i + 1]);
			redirectsOut = 2;
			//inputStruct->arguments[i] = "\0";
			//inputStruct->arguments[i + 1] = "\0";
		}
	}

	// check if command should run in background
	int  lastArg = (inputStruct->argCount) - 1;
	if (args && strcmp(inputStruct->arguments[lastArg], amp) == 0) {
		inputStruct->isBackground = 1;
		// remove from arg list
		//inputStruct->arguments[lastArg] = "\0";
	}

	// free memory from special args
	if (redirectsIn) {
		for (int i = redirectIndexIn; i < inputStruct->argCount; i++) {
			free(inputStruct->arguments[i]);
		}
	}
	else if (redirectsOut) {
		for (int i = redirectIndexOut; i < inputStruct->argCount; i++) {
			free(inputStruct->arguments[i]);
		}
	}
	else if (inputStruct->isBackground) {
		free(inputStruct->arguments[lastArg]);
	}

	// dec args to account for special chars and filenames
	inputStruct->argCount -= (redirectsIn + redirectsOut + inputStruct->isBackground);
}

void expandVariables(struct userInput* inputStruct) {
	char* findVar = "$$";
	char* substrPtr = NULL;
	char pidString[8] = { "\0" };
	// loop args and change $$ to processID
	for (int i = 0; i < inputStruct->argCount; i++) {

		while ((substrPtr = strstr(inputStruct->arguments[i], findVar)) != NULL) {
			// get length of word before $$
			int prefixLength = substrPtr - inputStruct->arguments[i];
			// get pid and convert to string
			int pid = getpid();
			sprintf(pidString, "%d", pid);
			
			// copy prefix and expanded variable
			char* newWord = calloc(strlen(inputStruct->arguments[i]) + strlen(pidString), sizeof(char));
			strncpy(newWord, inputStruct->arguments[i],  prefixLength);
			// append pid
			strcat(newWord, pidString);
			// append remaining, if exists
			if (substrPtr + 2 != '\0') {
				strcat(newWord, substrPtr + 2);
			}
			// replace the arg with the newWord
			free(inputStruct->arguments[i]);
			inputStruct->arguments[i] = newWord;
		}
	}
}

void expandVarInCommand(struct userInput* inputStruct) {
	char* findVar = "$$";
	char* substrPtr = NULL;
	char pidString[8] = { "\0" };
		while ((substrPtr = strstr(inputStruct->command, findVar)) != NULL) {
			// get length of word before $$
			int prefixLength = substrPtr - inputStruct->command;
			// get pid and convert to string
			int pid = getpid();
			sprintf(pidString, "%d", pid);
			// copy prefix and expanded variable
			char* newWord = calloc(strlen(inputStruct->command) + strlen(pidString), sizeof(char));
			strncpy(newWord, inputStruct->command,  prefixLength);
			// append pid
			strcat(newWord, pidString);
			// append remaining, if exists
			if (substrPtr + 2 != '\0') {
				strcat(newWord, substrPtr + 2);
			}
			// replace the command with the newWord
			free(inputStruct->command);
			inputStruct->command = newWord;
		}

}

void freeInputStruct(struct userInput* inputStruct)
{
	// free the command
	free(inputStruct->command);
	// loop the arg array and free each
	for (int i = 0; i < inputStruct->argCount; i++) {
		free(inputStruct->arguments[i]);
	}
	free(inputStruct->arguments);
	// free the filenames if exists
	if (inputStruct->redirectIn) {
		free(inputStruct->inputFile);
	}
	if (inputStruct->redirectOut) {
		free(inputStruct->outputFile);
	}
	free(inputStruct);
}
