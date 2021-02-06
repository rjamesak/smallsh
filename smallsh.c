#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

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

struct bgListNode {
	int pid;
	struct bgListNode* next;
	struct bgListNode* prev;
};

struct bgList {
	struct bgListNode* head;
	struct bgListNode* tail;
	int bgProcesses;
};

struct userInput* parseInput(char* inputLine);
void freeInputStruct(struct userInput* inputStruct);
void checkSpecialSymbols(struct userInput* inputStruct);
void expandVariables(struct userInput* inputStruct);
void expandVarInCommand(struct userInput* inputStruct);
void changeDirs(struct userInput* userInput);
void printDir();
struct bgList* addToBgList(struct bgList* list, int childPid);
struct bgList* removeFromBgList(struct bgList* list, struct bgListNode* deadNode);
//struct bgList* addBgList(struct bgList* head, struct bgList* tail, int childPid);
//struct bgList* removeNode(struct bgListNode* deadNode, struct bgList* head, struct bgList* tail);

int main(int argc, const char* argv[]) {
	// create a command line
	char* input = NULL;
	char* exitCmd = "exit";
	char* cd = "cd\0";
	//char* stat = "status\0";
	char* comment= "#\0";
	char* newline = "\n\0";
	struct bgList* bgPids = malloc(sizeof(struct bgList));
	bgPids->head = NULL;
	bgPids->tail = NULL;
	bgPids->bgProcesses = 0;
	int childStatus;
	//struct bgList* head = NULL;
	//struct bgList* tail = NULL;
	size_t inputLength; 
	//ssize_t nread;
	struct userInput* userInput = NULL;
	do {
		if (userInput) { 
			if (strncmp(userInput->command, cd, 2) == 0) {
				changeDirs(userInput);
			}
			//else if (strncmp(userInput->command, status, 6) == 0) {
				//printDir();
			//}
			else if (!(strncmp(userInput->command, comment, 1) == 0) && !(strncmp(userInput->command, newline, 1) == 0)) {
				//printf("not comment, cd, pwd, or newline. gonna fork exec here.\n");
				// fork new process
				pid_t childPid = fork();
				switch (childPid) {
				case -1:
					perror("fork() failed\n");
					exit(1);
					break;
				case 0:; // https://www.educative.io/edpresso/resolving-the-a-label-can-only-be-part-of-a-statement-error
					// run the command
					// build the arg array, size of args + 1 for command + 1 for NULL
					char** execArgs;
					execArgs = calloc(userInput->argCount + 2, sizeof(char*));
					execArgs[0] = userInput->command;
					for (int i = 1; i < userInput->argCount + 1; i++) {
						execArgs[i] = userInput->arguments[i - 1];
					}
					execArgs[userInput->argCount + 1] = NULL; // how to clean memory if doesn't return?
					// recommended use execlp() or execvp()
					execvp(userInput->command, execArgs);
					// exec returns if error
					perror("execvp error\n");
					exit(2);
					break;
				default:
					//parent process
					// if foreground, waitpid
					// foreground
					if (!userInput->isBackground) {
						childPid = waitpid(childPid, &childStatus, 0); // waits for child
					}
					// if background, add to background list, and print pid
					else if (userInput->isBackground) {
						// init list, add to bg list
						bgPids = addToBgList(bgPids, childPid);
						//head = addBgList(head, tail, childPid);
						bgPids->bgProcesses++;
						printf("Background Process %d started...\n", childPid);
					}
					break;
				}

				// be sure to terminate the child on fail
			} // end forking/exec code
			freeInputStruct(userInput); 
		}; // end if userInput

		// reap background processes and print when terminated here ? waitpid(...NOHANG...)
		// loop background list, waitpid NOHANG
		struct bgListNode* node = bgPids->head; // list iterator
		int removed = 0;
		while (node != NULL) {
			int childPid = waitpid(node->pid, &childStatus, WNOHANG);
			if (childPid) {
				printf("Background process %d terminated with status %d\n", childPid, childStatus);
				bgPids->bgProcesses--;
				// delete the node from the list
				bgPids = removeFromBgList(bgPids, node);
				//head = removeNode(node, head, tail);
				removed = 1;
			}
			if (!removed) {
				// continue with loop
				node = node->next;
			}
			else {
				// reset loop to beginning
				node = bgPids->head;
				removed = 0;
			}
		}


		printf(": ");
		getline(&input, &inputLength, stdin);
		userInput = parseInput(input);
		// action based on input
	} while (strncmp(userInput->command, exitCmd, 4) != 0);

	// clean input struct
	//free(input);
	free(input);
	freeInputStruct(userInput);
	free(bgPids);

	// kill any running ps or jobs

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
	//char* comment = "#";
	//char* newline = "\n";
	char* savePtr;
	char* token = strtok_r(inputLine, " ", &savePtr);
	// first input is the command
	// check for trailing newline, but not if first character
	// https://aticleworld.com/remove-trailing-newline-character-from-fgets/
	char* newline = strchr(token, '\n');
	if (newline  && newline != inputLine) {
		*newline = '\0';
	}
	parsedInput->command = strdup(token);

	// loop to store arguments
	parsedInput->arguments = calloc(513, sizeof(char*)); //create array of char ptrs
	int i = 0;
	while ((token = strtok_r(NULL, " ", &savePtr)) != NULL) {
		// replace newline with null unless it's the only arg
		newline = strchr(token, '\n');
		if (newline && newline != token) {
			*newline = '\0';
		}
		else if (newline) {
			break;
		}
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
	char* amp = "&\0";
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


void changeDirs(struct userInput* userInput) {
	// if no args, change to home env variable
	char* home = "HOME";
	char* homePath = NULL;
	//char* newPath  = NULL;
	int result = 0;
	if (!userInput->argCount) {
		homePath = getenv(home);
		result  = chdir(homePath);
		if (result == -1) {
			perror("unable to change to home path\n");
		}
		printDir();
	}
	// if  arg, change to the specified directory, absolute  or relative paths
	else  if (userInput->argCount) {
		result = chdir(userInput->arguments[0]);
		if (result == -1) {
			perror("unable to change to directory\n");
		}
		printDir();

	}

	return;
}

void printDir() {
	// https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
	char curPath[512];
	getcwd(curPath, sizeof(curPath));
	printf("current directory: %s\n", curPath);
	return;
}


//struct bgList* addBgList(struct bgList* head, struct bgList* tail, int childPid) {
//	// if empty, create new first node
//	if (head == NULL) {
//		struct bgList* node = malloc(sizeof(struct bgList));
//		node->pid = childPid;
//		node->next = NULL;
//		node->prev = NULL;
//		head = node;
//		tail = node;
//	}
//	else { // at least one node exists, grow the list
//		struct bgList* node = malloc(sizeof(struct bgList));
//		node->pid = childPid;
//		node->next = NULL;
//		// point prev tail to this new node
//		tail->next = node; 
//		tail = node; // set as new tail
//	}
//	return head;
//}

struct bgList* addToBgList(struct bgList* list, int childPid) {
	if (list->head == NULL) {
		struct bgListNode* node = malloc(sizeof(struct bgListNode));
		node->pid = childPid;
		node->next = NULL;
		node->prev = NULL;
		list->head = node;
		list->tail = node;
	}
	else { // at least one node exists, grow the list
		struct bgListNode* node = malloc(sizeof(struct bgListNode));
		node->pid = childPid;
		node->next = NULL;
		// point prev tail to this new node
		list->tail->next = node;
		list->tail = node; // set as new tail
	}
	return list;
}

struct bgList* removeFromBgList(struct bgList* list, struct bgListNode* deadNode) {
	if (deadNode != list->head) {
		deadNode->prev->next = deadNode->next;
		// connect next to prev
		if (deadNode != list->tail) {
			deadNode->next->prev = deadNode->prev;
		}
		// is tail node (and not head), set prev to new tail
		else {
			list->tail = deadNode->prev;
		}
	}
	// is head
	else {
		// head but not tail
		if (deadNode != list->tail) {
			list->head = deadNode->next;
			list->head->prev = NULL;
		}
		// head and tail
		else {
			list->head = NULL;
			list->tail = NULL;
		}
	}
	free(deadNode);
	return list;

}

//struct bgList* removeNode(struct bgList* deadNode, struct bgList* head, struct bgList* tail) {
//	// if has prev (not head), connect prev to next
//	if (deadNode != head) {
//		deadNode->prev->next = deadNode->next;
//		// connect next to prev
//		if (deadNode != tail) {
//			deadNode->next->prev = deadNode->prev;
//		}
//		// if tail, set prev to new tail
//		else {
//			tail = deadNode->prev;
//		}
//	}
//	// is head node
//	else {
//		if (deadNode != tail) {
//			head = deadNode->next;
//			head->prev = NULL;
//		}
//		// deadNode == head and tail (only node), set head and tail to null
//		else {
//			head = NULL;
//			tail = NULL;
//		}
//	}
//	free(deadNode);
//	return head;
//}

//  clean up the input  struct
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
