#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_CHARS 2048
#define MAX_ARGS 512

// Global variable to keep track of Foreground only mode. Global makes sense so it doesn't have to be passed aroundbetween the various functions that rely on it.
int isFGMode = 0;

// Function prototypes
void CatchSIGINT(int);
void CatchSIGTSTP(int);
void ChangeDir(char* []);
void ChangeSignals(struct sigaction*, struct sigaction*, int);
void Execute(char* []);
void Expand$$(char*); 
int FindRedirect(char* [], char*);
int FindBG(char* []);
void GetInput(char*);
void KillAll(pid_t[]);
void ParseInput(char*, char* []);
void ReapZombies(pid_t*, int*);
void RemoveArgs(char* [], int, int);
void Status(int*);

int main() {
	char input[MAX_CMD_CHARS]; // stores the input a user entered
	char* arguments[MAX_ARGS]; // stores all of the arguments a user entered
	pid_t spawnID = -5;
	int childExitMethod = -5;
	int inIdx = 0; // used to store index of arguments where a in "<" redirect symbol is used
	int outIdx = 0; // used to store index of arguments where a out ">" redirect symbol is used
	int fileI, fileO; // used for input/output files in redirects
	int bgIdx = 0; // used to store index of arguments where a "&" is used for background processes

	// 100 is an arbitrary reasonable value. Could lead to an issue if user runs more than 100 processes
	// but that seems unlikely.
	pid_t pIDS[100];
	int processCount = 0;
	memset(pIDS, -1, sizeof(pIDS));

	// signal handling sets SIGINT and SIGSTP behaviors
	struct sigaction SIGINT_action = {0};
	struct sigaction SIGTSTP_action = {0};
	struct sigaction IGNORE_action = {0};

	// change SIGINT behavior
	SIGINT_action.sa_handler = CatchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;

	// change SIGTSTP behavior
	SIGTSTP_action.sa_handler = CatchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;

	IGNORE_action.sa_handler = SIG_IGN;

	sigaction(SIGINT, &SIGINT_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// repeat the prompt forever until "exit" is entered
	while(1) {
		ReapZombies(pIDS, &processCount);
		GetInput(input);
		ParseInput(input, arguments);
		// user entered a comment "#" as the first character in input
		if(input[0] ==  '#' || input[0] == '\0') {
			// nothing happens
		}
		// user entered "exit"
		else if(strcmp(arguments[0], "exit") == 0) {
			KillAll(pIDS);
		} 
		// user entered "cd"
		else if(strcmp(arguments[0], "cd") == 0) {
			ChangeDir(arguments);	
		}
		// user entered "status"
		else if(strcmp(arguments[0], "status") == 0) {
			Status(&childExitMethod);
		}
		// no built-in command entered. call args using exec instead
		else {
			// store the location of the background symbol "&" in order to remove it from args
			int bgIdx = FindBG(arguments); 
			
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
							printf("cannot open %s for output\n", arguments[outIdx]);
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
							printf("cannot open %s for input\n", arguments[inIdx]);
							exit(1);
						}		
						else {
							// redirecting stdin to specified file
							dup2(fileI, 0);
							RemoveArgs(arguments, inIdx - 1, 2);
						}
					}
					// check if a background command was entered
					if(bgIdx) {
						// Remove the "&" from arguments
						RemoveArgs(arguments, bgIdx, 1);
						// If in foreground only mode, change signals to default
						if(isFGMode) {
							ChangeSignals(&SIGINT_action, &SIGTSTP_action, 1);	
						}
						// If not in foreground only mode, change signals to ignore
						// and set input/output to null (if relevant)
						else {
							ChangeSignals(&SIGINT_action, &SIGTSTP_action, 0);
							if(outIdx) {
								fileO = open("/dev/null", O_WRONLY);
								dup2(fileO, 1);
							}
							if(inIdx) {
								fileI = open("/dev/null", O_RDONLY);
								dup2(fileI, 0);
							}
						}
					}
					// Is not a background command; make sure signals are set to foreground settings
					else {
						ChangeSignals(&SIGINT_action, &SIGTSTP_action, 1);
					}

					Execute(arguments);				
					break;
				default: 
					// parent code
					// If not in foreground only mode and there is a background process,
					// print out the background PID
					if(!isFGMode && bgIdx) {
						fflush(stdout);
						printf("background PID is %d\n", spawnID);
						pIDS[processCount] = spawnID;
						processCount++;
					}
					// Not a background process - wait for child in foreground
					else {
						childExitMethod = -5;
						waitpid(spawnID, &childExitMethod, 0);
					}
					break;
			}
		}
	}
	
	return 0;
}

// Catches SIGINT signals and writes a message
void CatchSIGINT(int signo) {
	char* message = "terminated by signal 2\n";
	write(STDOUT_FILENO, message, 23);
}

// Catches SIGSTP signals and writes a message based on whether Foreground-only mode is enabled or not
void CatchSIGTSTP(int signo) {
	char* message1 = "Entering foreground-only mode (& is now ignored)\n";
	char* message2 = "Exiting foreground-only mode\n";
	if(isFGMode) {
		isFGMode = 0;
		write(STDOUT_FILENO, message2, 29);
	}	
	else {
		isFGMode = 1;
		write(STDOUT_FILENO, message1, 49);
	}
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

// Changes signal behavior based on whether foreground only mode are enabled or not
void ChangeSignals(struct sigaction* SIGINT_action, struct sigaction* SIGTSTP_action, int isFG) {
	// In foreground-only mode change SIGINT behavior to default (SIG_DFL)
	if(isFG) {
		SIGINT_action->sa_handler = SIG_DFL;
		SIGTSTP_action->sa_handler = SIG_IGN;
		sigaction(SIGINT, SIGINT_action, NULL);
		sigaction(SIGTSTP, SIGTSTP_action, NULL);
	}
	// Out of foreground-only mode ignore
	else {
		SIGINT_action->sa_handler = SIG_IGN;
		SIGTSTP_action->sa_handler = SIG_IGN;
		sigaction(SIGINT, SIGINT_action, NULL);
		sigaction(SIGTSTP, SIGTSTP_action, NULL);
	}
}

// Executes the given array of arguments being sure to flush stdout first
void Execute(char* args[]) {
	if(execvp(*args, args) < 0) {
		fflush(stdout);
		perror(args[0]);
		exit(1);
	}
}

// Expands string input instances of "$$" to the process ID
void Expand$$(char* input) {
	pid_t pid = getpid();
	// process ID's usually 5 digits, made mine 10 just in case
	char pidStr[10];
	memset(pidStr, '\0', sizeof(pidStr));
	sprintf(pidStr, "%d", pid);
	// look for instances of "$$" and replace with process ID's
	while(strstr(input, "$$") != NULL) {
		char* ptr = strstr(input, "$$");
		strcpy(ptr + strlen(pidStr), ptr + 2);
		strncpy(ptr, pidStr, strlen(pidStr));
	}
}

// Returns the index of where "&" is found or 0 if not found - used like a boolean
int FindBG(char* args[]) {
	int bgIdx = 0;
	// starting at index 1 because we need something before the & for it to be valid
	int i = 1;
	while(args[i] != NULL) {
		// if current argument is "&" and there's nothing after it
		if((strcmp(args[i], "&") == 0) && (args[i + 1] == NULL))
			bgIdx = i;
		i++;
	}
	return bgIdx;
}

// Finds the redirect symbol and returns the index after it (the file to read/write to)
// =
int FindRedirect(char* args[], char* symbol) {
	int redirectIdx = 0;
	// starting at index 1 because we don't want this to fire off if the first arg(index 0) is a redirect
	int i = 1;
	while(args[i] != NULL) {
		// if current argument is passed in symbol and there is a next argument after the symbol
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

// Kill all background and foreground processes
void KillAll(pid_t pIDS[]) {
	int i = 0, status = -5;
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
	// Expand "$$" in input string first
	Expand$$(input);
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

// Goes through list of background processes to determine which processes have ended and reports them to the user
// and removes them from the processID's array
void ReapZombies(pid_t pIDS[], int* processCount) {
	if(processCount > 0) {
		int i;
		int j;
		int childExitMethod = -5;
		int status = -1;
		// Go through array of active processes looking for any that have finished
		for(i = 0; i < *processCount; i++) {
			// Check if process has ended
			if(waitpid(pIDS[i], &childExitMethod, WNOHANG) != 0) {
				// Did process exit?
				if(WIFEXITED(childExitMethod) != 0) {
					status = WEXITSTATUS(childExitMethod);
					fflush(stdout);
					printf("background pid %d is done: exit value %d\n", pIDS[i], status);
				}
				// Did process terminate via signal?
				if(WIFSIGNALED(childExitMethod) != 0) {
					status = WTERMSIG(childExitMethod);
					fflush(stdout);
					printf("background pid %d is done: terminated by signal %d\n", pIDS[i], status);
				}
					// Process has been accounted for; decrease count and remove from array
				(*processCount)--;
				for(j = i; j < *processCount; j++) {
					pIDS[j] = pIDS[j+1];
				}
			}
		}

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

//void SigInit(struct sigaction* sigint, struct sigaction* sigstop

// Prints the exit status or terminating signal of the last foreground process
void Status(int* statCode) {
	// check if status was exited
	if(WIFEXITED(*statCode) != 0) {
		*statCode = WEXITSTATUS(*statCode);
		fflush(stdout);
		printf("exit value %d\n", *statCode);
	}
	// check if status was terminated by a signal
	else if(WIFSIGNALED(*statCode) != 0) {
		*statCode = WTERMSIG(*statCode);
		fflush(stdout);
		printf("terminated by signal %d\n", *statCode);
	}
}

