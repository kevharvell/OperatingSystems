#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_CHARS 2048
#define MAX_ARGS 512

void CatchSIGINT(int);
void GetInput(char*);
void KillAll(pid_t[]);
void ParseInput(char*, char* []);
void PrintArgs(char* []);

int main() {
	char input[MAX_CMD_CHARS]; 
	char* arguments[MAX_ARGS];

	// 100 is an arbitrary reasonable value. Could lead to an issue if user runs more than 100 processes
	pid_t pIDS[100];
	int processCount = 0;
	memset(pIDS, -1, sizeof(pIDS));

	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = CatchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	//SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);
	
	do {
		GetInput(input);
		ParseInput(input, arguments);
		PrintArgs(arguments);
		if(strcmp(arguments[0], "exit") == 0) {
			KillAll(pIDS);
		}
	} while(strcmp(input, "exit") != 0);
	
	// exit was entered
	

	return 0;
}

void CatchSIGINT(int signo) {
	char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}

void GetInput(char* input) {
	int numCharsEntered = -5; // how many chars we entered
	int currChar = -5; // tracks where we are when we print out ever char
	size_t bufferSize = 0; // holds how large the lalocated buffer is
	char* lineEntered = NULL; // points to a buffer alocated by getline() that holds our entered string + \n + \0
	
	while(1) {
		printf(": ");
		// get a line from the user
		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
		if(numCharsEntered == -1)
			clearerr(stdin);
		else
			break; // exit the loop - we've got input
	}
	// remove the trailing \n that getline adds
	lineEntered[strcspn(lineEntered, "\n")] = '\0';
	// copy entered line to input string
	strcpy(input, lineEntered);
	// free the memory allocated y getline() or else memory leak
	free(lineEntered);
	lineEntered = NULL;
}

void KillAll(pid_t pIDS[]) {
	int i = 0, status = -3;
	while(pIDS[i] != -1) {
		// kill process, then wait to clean resources
		kill(pIDS[i], SIGTERM);
		waitpid(pIDS[i], &status, 0);
		i++;
	}
	fflush(stdout);
	exit(0);
}

void ParseInput(char* input, char* args[]) {
	char* token;
	int argCounter = 0;
	token = strtok(input, " ");
	args[argCounter] = token;
	argCounter++;
	while(token != NULL) {
		token = strtok(NULL, " ");
		args[argCounter] = token;
		argCounter++;
	}
}

void PrintArgs(char* args[]) {
	printf("\n");
	int i = 0;
	while(args[i] != NULL) {
		printf("%s, ", args[i]);
		i++;
	}
	printf("\n");
}

