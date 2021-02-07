#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/***********************************************************
* STRUCTS
************************************************************/
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

/*******************************************************
* FUNCTION DECLARATIONS
*******************************************************/
struct userInput* parseInput(char* inputLine);
void freeInputStruct(struct userInput* inputStruct);
void checkSpecialSymbols(struct userInput* inputStruct);
void expandVariables(struct userInput* inputStruct);
void expandVarInCommand(struct userInput* inputStruct);
void changeDirs(struct userInput* userInput);
void printDir();
struct bgList* addToBgList(struct bgList* list, int childPid);
struct bgList* removeFromBgList(struct bgList* list, struct bgListNode* deadNode);
void displayStatus(int status);
struct bgList* checkBgPs(struct bgList* list, int* childStatus);
void reapTheKiddos(struct bgList* list);
void handle_SIGTSTP(int signo); // https://repl.it/@cs344/53singal2c
void toggleBackgroundMode();

/****************************************************************
	GLOBAL VARIABLES
********************************************************/
int g_bgMode = 1;

/********************************************************************************
	MAIN
******************************************************************************/
int main(int argc, const char* argv[]) {
	// create a command line
	char* input = NULL;
	char* exitCmd = "exit";
	char* cd = "cd\0";
	char* stat = "status\0";
	char* comment= "#\0";
	char* newline = "\n\0";
	struct bgList* bgPids = malloc(sizeof(struct bgList));
	bgPids->head = NULL;
	bgPids->tail = NULL;
	bgPids->bgProcesses = 0;
	int childStatus;
	int lastForegroundStatus = 0;
	size_t inputLength; 
	struct userInput* userInput = NULL;

	// signal handling
	// create the struct and fill it out
	//dbl braces to quiet error: https://stackoverflow.com/questions/13746033/how-to-repair-warning-missing-braces-around-initializer
	struct sigaction SIGINT_action = { {0} };
	struct sigaction SIGTSTP_action = { {0} };
	SIGINT_action.sa_handler = SIG_IGN;
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	// block all catchable signals while handling sigint
	sigfillset(&SIGINT_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART; // necessary?
	// install the signal handler
	sigaction(SIGINT, &SIGINT_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	do {
		if (userInput) { 
			if (strncmp(userInput->command, cd, 2) == 0) {
				changeDirs(userInput);
			}
			else if (strcmp(userInput->command, stat) == 0) {
			displayStatus(lastForegroundStatus);
			}
			else if (!(strncmp(userInput->command, comment, 1) == 0) && !(strncmp(userInput->command, newline, 1) == 0)) {
				// fork new process
				pid_t childPid = fork();
				switch (childPid) {
				case -1:
					perror("fork() failed\n");
					exit(1);
					break;
				case 0:; // https://www.educative.io/edpresso/resolving-the-a-label-can-only-be-part-of-a-statement-error
					// child process
					
					// children should ignore SIGTSTP
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigaction(SIGTSTP, &SIGTSTP_action, NULL);
					// if foreground, set default handling of sigint
					if (!userInput->isBackground) {
						SIGINT_action.sa_handler = SIG_DFL;
						sigaction(SIGINT, &SIGINT_action, NULL);
					}

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
					perror("failed to exec() command...\n");
					exit(1);
					break;
				default:
					//parent process
					// if foreground, waitpid
					// foreground
					if (!userInput->isBackground) {
						childPid = waitpid(childPid, &childStatus, 0); // waits for child
						if (WIFEXITED(childStatus)) {
							lastForegroundStatus = WEXITSTATUS(childStatus);
						}
						else if (WIFSIGNALED(childStatus)) {
							lastForegroundStatus = WTERMSIG(childStatus);
							printf("\nForeground process terminated with signal %d\n", lastForegroundStatus);
						}
					}
					// if background, add to background list, and print pid
					else if (userInput->isBackground) {
						
						// signal handling
						// create the struct and fill it out
						//struct sigaction SIGINT_action = { 0 };
						//SIGINT_action.sa_handler = handle_SIGINT;
						SIGINT_action.sa_handler = SIG_IGN;
						// block all catchable signals while handling sigint
						//sigfillset(&SIGINT_action.sa_mask);
						// SIGINT_action.sa_flags = SA_RESTART;
						// install the signal handler
						sigaction(SIGINT, &SIGINT_action, NULL);

						// init list, add to bg list
						bgPids = addToBgList(bgPids, childPid);
						//head = addBgList(head, tail, childPid);
						bgPids->bgProcesses++;
						printf("Background Process %d started...\n", childPid);
						fflush(stdin);
					}
					break;
				}

				// be sure to terminate the child on fail
			} // end forking/exec code
			freeInputStruct(userInput); 
		}; // end if userInput

		// reap background processes and print when terminated
		// move to fn, send bgPids and pointer to childStatus, return bgPids
		bgPids = checkBgPs(bgPids, &childStatus);

		printf(": ");
		fflush(stdin);
		getline(&input, &inputLength, stdin);
		userInput = parseInput(input);
		// action based on input
	} while (strncmp(userInput->command, exitCmd, 4) != 0);

	// clean input struct
	//free(input);
	free(input);
	freeInputStruct(userInput);

	// kill any running ps or jobs
	reapTheKiddos(bgPids);

	free(bgPids);
}
/****************************
	END MAIN
*****************************/

/****************************************************************
	FUNCTION IMPLEMENTATIONS
****************************************************************/
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
		// account for not background mode
	}

	// dec args to account for special chars and filenames
	inputStruct->argCount -= (redirectsIn + redirectsOut + inputStruct->isBackground);
	// account for foreground mode only
	if (!g_bgMode) {
		inputStruct->isBackground = 0;
	}
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
	fflush(stdin);
	return;
}


struct bgList* checkBgPs(struct bgList* list, int* childStatus) {

	// loop the bg process list and waitPid WNOHANG
	struct bgListNode* node = list->head; // list iterator
	int removed = 0;
	while (node != NULL) {
		int childPid = waitpid(node->pid, childStatus, WNOHANG);
		if (childPid) {
			if (WIFEXITED(*childStatus)) {
				printf("Background process %d terminated with status %d\n", childPid, WEXITSTATUS(*childStatus));
				fflush(stdin);
			}
			else if (WIFSIGNALED(*childStatus)) {
				printf("Background process %d terminated with signal %d\n", childPid, WTERMSIG(*childStatus));
				fflush(stdin);
			}
			// note removal
			list->bgProcesses--;
			// delete the node from the list
			list = removeFromBgList(list, node);
			removed = 1;
		}
		if (!removed) {
			// continue with loop
			node = node->next;
		}
		else {
			// reset loop to beginning
			node = list->head;
			removed = 0;
		}
	}
	return list;
}


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

void reapTheKiddos(struct bgList* list) {
	struct bgListNode* node = list->head;
	while (node != NULL) {
		// itr and kill pids
		kill(node->pid, SIGKILL);
		node = node->next;
	}
}

void displayStatus(int status) {
	printf("exit value: %d\n", status);
	fflush(stdin);
}

void toggleBackgroundMode() {
	char* ignored = "& is now ignored\n";
	char* notIgnored = "& is not ignored\n";

	if (g_bgMode) {
		g_bgMode = 0;
		write(STDOUT_FILENO, ignored, 18);
	}
	else {
		g_bgMode = 1;
		write(STDOUT_FILENO, notIgnored, 18);
	}
}

void handle_SIGTSTP(int signo) {
	// toggle background mode
	// print message saying & will be ignored
	char* message = "\ntoggling background mode\n";
	write(STDOUT_FILENO, message, 27);
	toggleBackgroundMode();

	//char* message = "foreground process terminated by signal ";
	//char sigNum[7];
	//// convert number to char
	//int result;
	//int counter = 0;
	//while ((result = signo / 10) > 0) {
	//	// get remainder into char
	//	sigNum[counter] = (signo % 10) + 48;
	//	counter++;
	//	// check next number;
	//	signo = result;
	//}
	//// get last number
	//sigNum[counter] = (signo % 10) + 48;
	//counter++;

	//int sigMsgSize = counter + 1;
	//// write message and signal number 
	//write(STDOUT_FILENO, message, 41);
	//// write the signo backwards
	//char sigMessage[sigMsgSize];
	//for (int i = 0; counter >= 0; i++) {
	//	sigMessage[i] = sigNum[counter];
	//	counter--;
	//}
	//char* newline = "\n";
	//write(STDOUT_FILENO, sigMessage, sigMsgSize);
	//write(STDOUT_FILENO, newline, 2);
	char* shellPrompt = ": ";
	write(STDOUT_FILENO, shellPrompt, 3);
}

//  clean up the input struct
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
