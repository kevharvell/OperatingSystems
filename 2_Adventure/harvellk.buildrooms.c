#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>


struct Room {
	char name[50];
};

int numInArr(int, int*);
void CreateRooms(struct Room*);
int IsGraphFull();
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
	struct Room rooms[7];

	CreateRooms(rooms);
	

	//printf("These are the rooms in the Rooms array:\n");
	//for(i = 0; i < 7; i++) {
	//	printf("%s\n", rooms[i].name);
	//}
	
	// Create all connections in graph
	//while (IsGraphFull() == 0) {
	//	AddRandomConnection();
	//}

	return 0;
}

int numInArr(int num, int* arr) {
	int i;
	for(i = 0; i < 7; i++) {
		if(arr[i] == num) {
			return 1;
		}
	}
	return 0;
}

// Creates a random room in the room directory folder
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

	int numCount = 0;
	while(numCount < 7) {
		// pick a number between 0 and 9
		int randNum = rand() % 10;
		printf("random num is: %d\n", randNum);
		if(!numInArr(randNum, randNumsArr)) {
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
