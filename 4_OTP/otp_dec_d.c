#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Error function used for reporting issues
void error(const char *msg) { perror(msg); exit(0); }

// function prototypes
void Decrypt(char*, char*);
char* GetKey(char*);
char* GetText(char*);
void ReapZombies(pid_t[], int[], int);
void ReceiveSocket(char*, int);
void SendAck(int);
void SendSocket(char*, int);

int main(int argc, char* argv[]) {
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	// initialize connections number to 0
	pid_t pidArr[5];
	int connections[5];
	int numConnections = 0;

	// Check usage & args
	if(argc < 2) { fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1); }

	// Set up the server address struct
 	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");
	// Enable the socket to begin listening
	// Connect socket to port
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	
	while(1) {
		// Clean up zombie processes that have completed
		ReapZombies(pidArr, connections, numConnections);
		
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		// fork process
		int spawnID = fork();
		switch(spawnID) {
			case -1:
				perror("child process spawn failure\n");
				exit(1);
				break;
			case 0:
				// child code
				// send out a unique identifier so client knows that we are otp_dec_d
				SendAck(establishedConnectionFD);
				
				// store the received message
				char inText[300000];
				memset(inText, '\0', sizeof(inText));

				ReceiveSocket(inText, establishedConnectionFD);
				
				// get text and key contents from inText
				char* text = malloc(150000*sizeof(char));
				char* key = malloc(150000*sizeof(char));
				memset(text, '\0', sizeof(text));
				memset(key, '\0', sizeof(key));

				strcpy(text, GetText(inText));
				strcpy(key, GetKey(inText));
				
				// Decrypt text using key
				Decrypt(key, text);			
				SendSocket(text, establishedConnectionFD);
				free(text);
				free(key);
				exit(0);
				break;
			default:
				pidArr[numConnections] = spawnID;
				connections[numConnections] = establishedConnectionFD;
				numConnections++;
		}
	}
	// close listening socket
	close(listenSocketFD);
	return 0;
}

// Decrypt
void Decrypt(char* key, char* text) {
	int i;
	int charCode;
	char* decryptText = malloc(strlen(text) * sizeof(char));
	memset(decryptText, '\0', sizeof(decryptText));
	for(i = 0; i < strlen(text); i++) {
		// look for spaces in key/text
		// these spaces are encoded to '@' symbol (charCode 64).
		// I do this because in my otp_enc.c file I create the key randomly including '@' that
		// are converted to spaces. The @ symbol was chosen because it's easier to make random
		// numbers between 64-90 than to do 65-90 and then randomly get a space (32) somehow.
		if(text[i] == 32) {
			text[i] = 64;
		}
		if(key[i] == 32) {
			key[i] = 64;
		}

		// Create encrypted charCode using text - key % 27
		// Possible negative charCode needs to be accounted for; if negative, 27 added.
		int temp = text[i] - key[i];
		temp = temp % 27;
		if(temp < 0)
			temp += 27;
		charCode = temp + 64;

		// turn key and charCode characters back into space from '@' to maintain original data
		// text not turned back because it is going to be overwritten by the encrypted text next.
		if(key[i] == 64) {
			key[i] = 32;
		}
		if(charCode == 64) {
			charCode = 32;
		}

		decryptText[i] = charCode;
	}
	strcpy(text, decryptText);
	free(decryptText);
}

// GetKey
// inText is a string with the key and text together separated by '@'. This function looks for the '@'
// and assigns everything before it to a key variable
char* GetKey(char* inText) {
	char key[150000];
	int i;
	for(i = 0; i < strlen(inText); i++) {
		if(inText[i] == '@') {
			strncpy(key, inText, i);
		}
	}
	return key;
}

// GetText
// inText is a string with the key and text together separated by '@'. This function looks for the '@' 
// and assigns everything after it to a text variable
char* GetText(char* inText) {
	char text[150000];
	int i;
	for(i = 0; i < strlen(inText); i++) {
		if(inText[i] == '@') {
			strcpy(text, inText + i + 1);
		}
	}
	return text;
}

// Goes through list of background processes to determine which processes have ended and reports them to the user
// and removes them from the processID's array
void ReapZombies(pid_t pidArr[], int connections[], int numConnections) {
	if((numConnections) > 0) {
		int i;
		int j;
		int childExitMethod = -5;
	
		// Go through array of active processes looking for any that have finished
		for(i = 0; i < numConnections; i++) {
			// Check if process has ended
			if(waitpid(pidArr[i], &childExitMethod, WNOHANG) != 0) {
				close(connections[i]);
				// remove the process from pidArray by shifting remaining processes left
				// in the array
				for(j = i; j < numConnections; j++) {
					pidArr[j] = pidArr[j+1];
					connections[j] = connections[j+1];
				}
			}
		}
	}
}
// ReceiveSocket
// Calls recv() looking for 299,999 characters and stores in the received text pointer.
// Then checks to see that all characters were received. 
void ReceiveSocket(char* text, int socketFD) {
	int charsRead;
	// clear text string to write to it
	memset(text, '\0', sizeof(text));
	// read client message from socket
	charsRead = recv(socketFD, text, 299999, 0);

	while(charsRead != 299999) {
		charsRead += recv(socketFD, text + charsRead, 299999, 0);
		if(charsRead < 0)
			error("ERROR: reading client message from socket");
	}
}

// SendAck
// Sends a single char 'x' to the client to identify itself as otp_dec_d
void SendAck(int establishedConnectionFD) {
	char* code = "x";
	int charsWritten;
	charsWritten = send(establishedConnectionFD, code, 1, 0);
}

// SendSocket
// Calls send() to send outText to specified server.
// Then checks to see that all characters were sent.
void SendSocket(char* outText, int socketFD) {
	int charsWritten;
	int length = strlen(outText);
	
	// write to server
	charsWritten = send(socketFD, outText, 299999, 0); 

	while(charsWritten != 299999) {
		// read client message from socket
		charsWritten += send(socketFD, outText + charsWritten, 299999, 0);
		if(charsWritten < 0) error("ERROR: reading client message from socket");
	}
}
