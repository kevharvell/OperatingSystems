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
int IsInputValid(char*);
char* FileToText(char*);
int IsValidAck(int);
void ReceiveSocket(char*, int);
void SendSocket(char*, int);

int main(int argc, char *argv[]) {
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;

	// Check usage & args
	if(argc < 4) { fprintf(stderr, "Not enough arguments."); exit(0); }

	// Get text from file
	char* text = FileToText(argv[1]);

	// Get key from file
	char* key = FileToText(argv[2]);

	// Check to see that the text is larger than the key. If not, print an error and free
	// allocated memory
	if(strlen(text) > strlen(key)) {
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		free(text);
		free(key);
		exit(1);
	}
	
	// Check to see if either text or key contain invalid inputs. If they do contain invalid
	// inputs, free allocated memory and exit program.
	if(!IsInputValid(text) || !IsInputValid(key)) {
		fprintf(stderr, "Error: input contains bad characters\n");
		free(text);
		free(key);
		exit(1);
	}

	// Get rid of the \n at the end of input and change to \0
	key[strcspn(key, "\n")] = '\0';
	text[strcspn(text, "\n")] = '\0';

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if(serverHostInfo == NULL) {fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if(socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	// Connect socket to address
	if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
		error("CLIENT: ERROR connecting");

	// Check if the connection receives valid acknowledgment. If not free memory, print error
	// and exit with value of 2
	if(!IsValidAck(socketFD)) {
		free(text);
		free(key);
		fprintf(stderr, "Error: could not contact otp_dec_d on port %s", argv[3]);
		exit(2);	
	}
	
	// create a outText string to store both the key and text separated by a @ symbol. 
	// 100,000 is an arbitrary length to hold both key and text. This could pose an issue
	// with very large text sizes beyond 100,000.
	char outText[300000];
	memset(outText, '\0', sizeof(outText));

	strcpy(outText, key);
	strcpy(outText + strlen(key), "@");
	strcpy(outText + strlen(key) + 1, text);

	// send outText string to be encoded on server
	SendSocket(outText, socketFD);

	// recieve the ciphertext from server
	memset(text, '\0', sizeof(text));
	ReceiveSocket(text, socketFD);
	printf("%s\n", text);

	close(socketFD); // Close the socket
	free(text);
	free(key);
	return 0;
}

// IsInputValid
// A bool function that checks string input to see if it is only A-Z and ' '. If it is, this function
// returns 1, otherwise 0. 
int IsInputValid(char* input) {
	int validBool = 1; 
	int i;
	for(i = 0; i < strlen(input) - 1; i++) {
		// check if each character is A-Z or ' '. If not return 0;
		if((input[i] < 65 || input[i] > 90) && input[i] != ' ') {
			validBool = 0;		
		}	
	}
	return validBool;
}

// FileToText
// Takes in a file name, converts text content in file to character array and returns that array
// Code based on:
// https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
// Thank you @Nils Pipenbrinck
char* FileToText(char* fileName) {
	FILE* fptr = fopen(fileName, "r");
	if(fptr) {
		fseek(fptr, 0, SEEK_END);
		long length = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);
		// add 1 to length for null terminator
		char* text = malloc(length + 1); 
		memset(text, '\0', sizeof(text));
		fread(text, length, 1, fptr);
		fclose(fptr);
		return text;
	}
}

// IsValidAck
// Makes a recv() call and checks to see if 'x' was returned. 'x' is a signal that the client
// is communicating with the wrong daemon
int IsValidAck(int socketFD) {
	int charsRead;
	char buffer[1];
	int isValidBool = 1;

	buffer[0] = '\0';

	charsRead = recv(socketFD, buffer, 1, 0);
	if(buffer[0] == 'a') {
		isValidBool = 0;
	}
	return isValidBool;
}

// ReceiveSocket
// Calls recv() looking for 299,999 characters and stores in the received text pointer.
// Then checks to see that all characters were received. 
void ReceiveSocket(char* text, int socketFD) {
	int charsRead;
	char tempText[300000];
	// clear text string to write to it
	memset(tempText, '\0', sizeof(tempText));
	// read client message from socket
	charsRead = recv(socketFD, tempText, 299999, 0);

	while(charsRead != 299999) {
		charsRead += recv(socketFD, tempText + charsRead, 299999, 0);
		if(charsRead < 0)
			error("ERROR: reading client message from socket");
	}
	strcpy(text, tempText);
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
