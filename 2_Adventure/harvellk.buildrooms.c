#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>


struct Room {
	char name[50];
};

void MakeRoomDir(char*);
void CreateRoom(char*);
int IsGraphFull();
int FileExists(char*);
void AddRandomConnection();
struct Room GetRandomRoom();
int CanAddConnectionFrom(struct Room);
int ConnectionAlreadyExists(struct Room, struct Room);
void ConnectRoom(struct Room, struct Room);
int IsSameRoom(struct Room, struct Room);

int main() {
	// seed for random
	srand(time(0));
	char roomFolder[25];
	int i;

	MakeRoomDir(roomFolder);
	for(i = 0; i < 7; i++) {
		CreateRoom(roomFolder);
	}
	// Create all connections in graph
	//while (IsGraphFull() == 0) {
	//	AddRandomConnection();
	//}




	return 0;
}

// Makes the rooms directory
void MakeRoomDir(char* roomFolder) {
	// make the rooms folder
	pid_t pidInt = getpid();
	// pid needs to be converted into a string in order to combine with roomFolder name
	// my pid's are 5 digits, but I made the string 10 characters just in case
	char pidStr[10];
	sprintf(pidStr, "%d", pidInt);
	strcpy(roomFolder, "harvellk.rooms.");
	strcat(roomFolder, pidStr);
	printf("Process ID: %d\n", pidInt);
	mkdir(roomFolder);
        printf("%s created\n", roomFolder);	
}

// Creates a random room in the room directory folder
void CreateRoom(char* roomFolder) {
	// possible room names to select from	
	char names[10][9] = { 
		"Dorm", 
		"Kitchen", 
		"Bedroom", 
		"Library", 
		"Storage", 
		"Forge", 
		"Chapel", 
		"Bath",
		"Armory",
		"Dungeon"
	};
	// pick a number between 0 and 9
	int randNum = rand() % 10;
	printf("random num is: %d\n", randNum);

	// create the file path for the room file
	char filePath[35];
	sprintf(filePath, "./%s/%s", roomFolder, names[randNum]); 
	printf("filePath is: %s\n", filePath);
	
	// check to see that the file exists before creating (in order to avoid duplicate rooms)
	if(!FileExists(filePath)) {
		FILE *fptr;
		fptr = fopen(filePath, "w+");
	}
	// file exists, so try again recursively
	else {
		CreateRoom(roomFolder);
	}
}

// Checks to see if a file exists (something that will be done a lot and deserves its own function)
int FileExists(char* filePath) {
	FILE* file;
	if (file = fopen(filePath, "r")) {
		fclose(file);
		return 1;
	}
	return 0;
}


// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int IsGraphFull() {
	return 0;
}

// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection() {
	struct Room A; // Maybe a struct, maybe global arrays of ints
	struct Room B;

	while(1) {
		A = GetRandomRoom();

		if (CanAddConnectionFrom(A) == 1)
			break;
	}

	do {
		B = GetRandomRoom();
	}
	while(CanAddConnectionFrom(B) == 0 || IsSameRoom(A, B) == 1 || ConnectionAlreadyExists(A, B) == 1);

	ConnectRoom(A, B); // TODO: Add this connection to the real variables
	ConnectRoom(B, A); // because this A and B will be destroyed when this function terminates
}

// Returns a random Room, does NOT validate if connections can be added
struct Room GetRandomRoom() {
	struct Room A;
	return A;
}

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
int CanAddConnectionFrom(struct Room x) {
	return 0;
}

// Connects Rooms x and y together, dows not check if this connection is valid
void ConnectRoom(struct Room x, struct Room y) {

}

// Returns true if a connection from Room x to Room y already exists, false otherwise
int ConnectionAlreadyExists(struct Room x, struct Room y) {
	return 0;
}

// Returns true if Rooms x and y are the same Room, false otherwise
int IsSameRoom(struct Room x, struct Room y) {
	return 0;
}
