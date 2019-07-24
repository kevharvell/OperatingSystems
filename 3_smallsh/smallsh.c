#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_CHARS 2048
#define MAX_ARGS 512

void CatchSIGINT(int);
void ChangeDir(char* []);
void Execute(char* []);
int FindRedirect(char* [], char*);
void GetInput(char*);
int IsBackground(char* []);
void KillAll(pid_t[]);
void ParseInput(char*, char* []);
void PrintArgs(char* []);
void RemoveArgs(char* [], int, int);
void Status();

int main() {
	char input[MAX_CMD_CHARS]; // stores the input a user entered
	char* arguments[MAX_ARGS]; // stores all of the arguments a user entered
	pid_t spawnID = -5;
	int childExitMethod = -5;
	int inIdx = 0; // used to store index of arguments where a in "<" redirect symbol is used
	int outIdx = 0; // used to store index of arguments where a out ">" redirect symbol is used
	int fileI, fileO; // used for input/output files in redirects

	// 100 is an arbitrary reasonable value. Could lead to an issue if user runs more than 100 processes
	// but that seems unlikely.
	pid_t pIDS[100];
	int processCount = 0;
	memset(pIDS, -1, sizeof(pIDS));

	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = CatchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	//SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);
	
	while(1) {
		GetInput(input);
		ParseInput(input, arguments);
		// user entered "exit"
		if(strcmp(arguments[0], "exit") == 0) {
			KillAll(pIDS);
		} 
		// user entered "cd"
		else if(strcmp(arguments[0], "cd") == 0) {
			ChangeDir(arguments);	
		}
		// user entered "status"
		else if(strcmp(arguments[0], "status") == 0) {
			Status();
		}
		// user entered a comment "#" as the first character in input
		else if(input[0] ==  '#') {
			// nothing happens
		}
		// no built-in command entered. call args using exec instead
		else {
			// create the child here
			spawnID = fork();
			switch(spawnID) {
				case -1:
					perror("child process spawn failure\n");
					exit(1);
					break;
				case 0:
					// child code
					// check if out redirect
					outIdx = FindRedirect(arguments, ">");
					if(outIdx) {
						// Try to open or create the specified file
						fileO = open(arguments[outIdx], O_CREAT | O_TRUNC | O_WRONLY, 0600);
						if(fileO < 0) {
							perror("could not open file\n");
							exit(1);
						}
						else {
							// redirecting stdout to specified file
							dup2(fileO, 1);
							// remove the redirect and file name arguments
							RemoveArgs(arguments, outIdx - 1, 2);
						}
					}

					// check if in redirect
					inIdx = FindRedirect(arguments, "<");
					if(inIdx) {
						// Try to open the file to read from
						fileI = open(arguments[inIdx], O_RDONLY, 0600);
						if(fileI < 0) {
							perror("could not open file\n");
							exit(1);
						}		
						else {
							// redirecting stdin to specified file
							dup2(fileI, 0);
							RemoveArgs(arguments, inIdx - 1, 2);
						}
					}
					if(IsBackground(arguments))
						printf("THIS IS A BG COMMAND\n");
					Execute(arguments);				
					break;
				default: 
					// parent code
					waitpid(spawnID, &childExitMethod, 0);
					break;
			}
		}
	}
	
	return 0;
}

// Catches SIGINT signals and writes a message
void CatchSIGINT(int signo) {
	char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}

// Implements the "cd" command by changing the directory if specified. If not, defaults to environment
// HOME directory
void ChangeDir(char* args[]) {
	int status;
	if(args[1] == NULL) {
		chdir(getenv("HOME"));
	}
	else {
		status = chdir(args[1]);
		if(status == -1) {
			printf("path %s does not exist.\n", args[1]);
			fflush(stdout);
		}
	}
}

// Executes the given array of arguments being sure to flush stdout first
void Execute(char* args[]) {
	if(execvp(*args, args) < 0) {
		fflush(stdout);
		perror("exec call failure");
		exit(1);
	}
}

// Finds the redirect symbol and returns the index after it (the file to read/write to)
int FindRedirect(char* args[], char* symbol) {
	int redirectIdx = 0;
	// starting at index 1 because we don't want this to fire off if the first arg(index 0) is a redirect
	int i = 1;
	while(args[i] != NULL) {
		if((strcmp(args[i], symbol) == 0) && (args[i + 1] != NULL))
			redirectIdx = i + 1;
		i++;
	}
	return redirectIdx;
}

// Get the input from the user and store it in the passed in input variable
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

// Checks to see if the commands entered are background commands
int IsBackground(char* args[]) {
	// Find the length of the args array
	int length = 0;
	while(args[length] != NULL) {
		length++;
	}
	if(strcmp(args[length - 1], "&") != 0)
		length = 0;
	return length;
}

// Kill all background and foreground processes
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

// Take given input and parse it by spaces into an arguments array
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

// Used for testing
void PrintArgs(char* args[]) {
	int i;
	printf("\n");
	while(args[i] != NULL) {
		printf("%s\n", args[i]);
		i++;
	}
}

// Removes a number of arguments(indicated by numArgsRemoved) starting at the specified index
void RemoveArgs(char* args[], int index, int numArgsRemoved) {
	int i;
	// repeat removing from array for as many args that need to be removed
	for(i = 0; i < numArgsRemoved; i++) {
		int j = index;
		while(args[j+1] != NULL) {
			args[j] = args[j+1];
			j++;
		}
		args[j] = NULL;
	}
}

// Prints the exit status or terminating signal of the last foreground process
void Status() {
	int statCode;
	// check if status was exited
	if(WIFEXITED(statCode) != 0) {
		statCode = WEXITSTATUS(statCode);
		fflush(stdout);
		printf("exit value %d\n", statCode);
	}
	// check if status was terminated by a signal
	else if(WIFSIGNALED(statCode) != 0) {
		statCode = WTERMSIG(statCode);
		fflush(stdout);
		printf("terminated by signal %d\n", statCode);
	}
}

