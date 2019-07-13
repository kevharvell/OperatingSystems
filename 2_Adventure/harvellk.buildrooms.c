#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>


struct Room {
	char name[9];
	char type[11];
	char connections[6][9];
	int numConnections;
};

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
	struct Room rooms[7];

	CreateRooms(rooms);
	


	printf("here's a random room: \n");
	struct Room* randRoom = GetRandomRoom(rooms);
	printf("%s\n", randRoom->name);
	
	// Create all connections in graph
	while (IsGraphFull(rooms) == 0) {
		AddRandomConnection(rooms);
	}
	

	printf("These are the rooms in the Rooms array:\n");
	for(i = 0; i < 7; i++) {
		printf("Name: %s, Type: %s, # of Conns: %d\n", rooms[i].name, rooms[i].type, rooms[i].numConnections);
		printf("Connections: ");
		int j;
		for(j = 0; j < 6; j++) {
			printf("%s, ", rooms[i].connections[j]);
		}
		printf("\n");
	}

	if(!IsGraphFull(rooms)) {
		printf("Graph is not full!\n");
	}
	
	return 0;
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
	int randNumsArr[7];
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

	// Create an array of random numbers
	int numCount = 0;
	while(numCount < 7) {
		// pick a number between 0 and 9
		int randNum = rand() % 10;
		// Check to make sure the chosen random number isn't already in the array. 
		// If not, add it to the random number array
		if(!numInArr(randNum, randNumsArr, 7)) {
			randNumsArr[numCount] = randNum;
			numCount++;
		}
	}

	int i = 0;
	printf("random nums are: ");
	for(i = 0; i < 7; i++) {
		printf("%d, ", randNumsArr[i]);
	}
	printf("\n");

	// Based on the random numbers, use the names array to put names into the Rooms array.
	// Also use this loop to add room types and set numConnections to 0
	for(i = 0; i < 7; i++) {
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
		for(j = 0; j < 6; j++) {
			strcpy(rooms[i].connections[j], "");
		}
	}
}


// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int IsGraphFull(struct Room* rooms) {
	int i;
	int isFull = 1;
	for(i = 0; i < 7; i++) {
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
	int randNum = rand() % 7;
	return &rooms[randNum];
}

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
int CanAddConnectionFrom(struct Room* x) {
	int canAdd = 0;
	if(x->numConnections < 6)
		canAdd = 1;
	return canAdd;
}

// Connects Rooms x and y together, dows not check if this connection is valid
void ConnectRoom(struct Room* x, struct Room* y) {
	int numCons = x->numConnections;
	strcpy(x->connections[numCons], y->name);
	x->numConnections++;
}

// Returns true if a connection from Room x to Room y already exists, false otherwise
int ConnectionAlreadyExists(struct Room* x, struct Room* y) {
	int conExists = 0;
	int i;
	// loop through connections array of x looking for the name of y
	for(i = 0; i < 6; i++) {
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
