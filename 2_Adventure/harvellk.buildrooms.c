#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#define NUM_ROOMS 7
#define MAX_CONNECTIONS 6
#define MAX_NAME_SIZE 9

struct Room {
	char name[MAX_NAME_SIZE];
	char type[11];
	char connections[MAX_CONNECTIONS][MAX_NAME_SIZE];
	int numConnections;
};

void MakeRoomDir(struct Room*, char *);
int numInArr(int, int*, int);
void CreateRooms(struct Room*);
int IsGraphFull(struct Room*);
void AddRandomConnection();
struct Room* GetRandomRoom(struct Room*);
int CanAddConnectionFrom(struct Room*);
int ConnectionAlreadyExists(struct Room*, struct Room*);
void ConnectRoom(struct Room*, struct Room*);
int IsSameRoom(struct Room*, struct Room*);

int main() {
	// seed for random
	srand(time(0));
	char roomFolder[25];
	int i;
	struct Room rooms[NUM_ROOMS];

	// Create Room struct array first
	CreateRooms(rooms);
	// Create all connections in graph
	while (IsGraphFull(rooms) == 0) {
		AddRandomConnection(rooms);
	}
	// Then create folder/files with Room struct array info
	MakeRoomDir(rooms, roomFolder);
	
	return 0;
}

void MakeRoomDir(struct Room* rooms , char* roomFolder) {
	
	// make the rooms folder
	pid_t pidInt = getpid();
	// pid needs to be converted into a string in order to combine with roomFolder name
	// my pid's are 5 digits, but I made the string 10 characters just in case
	char pidStr[10];
	sprintf(pidStr, "%d", pidInt);
	strcpy(roomFolder, "harvellk.rooms.");
	strcat(roomFolder, pidStr);
	mkdir(roomFolder, 0700);

	// Create room files in inside rooms folder
	int i;
	for(i = 0; i < NUM_ROOMS; i++) {
		// create the file path for the room file
		char filePath[35];
		sprintf(filePath, "./%s/%s", roomFolder, rooms[i].name);
		FILE *fptr;
		fptr = fopen(filePath, "w+");
		// populate room file with room struct info
		// add name
		fprintf(fptr, "ROOM NAME: %s\n", rooms[i].name);
		// add connections
		int j;
		for (j = 0; j < rooms[i].numConnections; j++) {
			fprintf(fptr, "CONNECTION %d: %s\n", j+1, rooms[i].connections[j]);
		}
		fprintf(fptr, "ROOM TYPE: %s", rooms[i].type);
		fclose(fptr);
	}
}

// Checks to see if a number is in an array of specified length
int numInArr(int num, int* arr, int length) {
	int i;
	for(i = 0; i < length; i++) {
		if(arr[i] == num) {
			return 1;
		}
	}
	return 0;
}

// Creates and initializes random rooms in a Room struct array
void CreateRooms(struct Room* rooms) {
	// array to store random numbers
	int randNumsArr[NUM_ROOMS];
	// possible room names to select from	
	char names[10][MAX_NAME_SIZE] = { 
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

	// Create an array of random numbers
	int numCount = 0;
	while(numCount < NUM_ROOMS) {
		// pick a number between 0 and 9
		int randNum = rand() % 10;
		// Check to make sure the chosen random number isn't already in the array. 
		// If not, add it to the random number array
		if(!numInArr(randNum, randNumsArr, NUM_ROOMS)) {
			randNumsArr[numCount] = randNum;
			numCount++;
		}
	}


	// Based on the random numbers, use the names array to put names into the Rooms array.
	// Also use this loop to add room types and set numConnections to 0
	int i = 0;
	for(i = 0; i < NUM_ROOMS; i++) {
		// Name assignment
		int num = randNumsArr[i];
		strcpy(rooms[i].name, names[num]);
		// Room type assignment
		if(i == 0) {
			strcpy(rooms[i].type, "START_ROOM");
		} else if(i == 1) {
			strcpy(rooms[i].type, "END_ROOM");
		} else {
			strcpy(rooms[i].type, "MID_ROOM");
		}
		// Set number of connections of all rooms to 0
		rooms[i].numConnections = 0;

		int j;
		for(j = 0; j < MAX_CONNECTIONS; j++) {
			strcpy(rooms[i].connections[j], "");
		}
	}
}


// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int IsGraphFull(struct Room* rooms) {
	int i;
	int isFull = 1;
	for(i = 0; i < NUM_ROOMS; i++) {
		if(rooms[i].numConnections < 3) {
			isFull = 0;
		}
	}
	return isFull;
}

// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection(struct Room* rooms) {
	struct Room* A; // Maybe a struct, maybe global arrays of ints
	struct Room* B;

	while(1) {
		A = GetRandomRoom(rooms);
		
		if (CanAddConnectionFrom(A) == 1)
			break;
	}

	do {
		B = GetRandomRoom(rooms);
	}
	while(CanAddConnectionFrom(B) == 0 || IsSameRoom(A, B) == 1 || ConnectionAlreadyExists(A, B) == 1);

	ConnectRoom(A, B); 
	ConnectRoom(B, A); 
}

// Returns a random Room, does NOT validate if connections can be added
struct Room* GetRandomRoom(struct Room* rooms) {
	int randNum = rand() % NUM_ROOMS;
	return &rooms[randNum];
}

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
int CanAddConnectionFrom(struct Room* x) {
	int canAdd = 0;
	if(x->numConnections < MAX_CONNECTIONS)
		canAdd = 1;
	return canAdd;
}

// Connects Rooms x and y together, dows not check if this connection is valid
void ConnectRoom(struct Room* x, struct Room* y) {
	int numCons = x->numConnections;
	strcpy(x->connections[numCons], y->name);
	(x->numConnections)++;
}

// Returns true if a connection from Room x to Room y already exists, false otherwise
int ConnectionAlreadyExists(struct Room* x, struct Room* y) {
	int conExists = 0;
	int i;
	// loop through connections array of x looking for the name of y
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		if(strcmp(x->connections[i], y->name) == 0)
			conExists = 1;
	}
	return conExists;
}

// Returns true if Rooms x and y are the same Room, false otherwise
int IsSameRoom(struct Room* x, struct Room* y) {
	int isSame = 0;
	if(strcmp(x->name, y->name) == 0)
		isSame = 1;
	return isSame;
}
