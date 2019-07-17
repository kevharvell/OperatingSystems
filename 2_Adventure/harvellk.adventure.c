#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_ROOMS 7
#define MAX_CONNECTIONS 6
#define MAX_NAME_SIZE 9

struct Room {
	char name[MAX_NAME_SIZE];
	char type[11];
	char connections[MAX_CONNECTIONS][MAX_NAME_SIZE];
	int numConnections;
};

int IsEndRoom(struct Room*);
struct Room* FindStartRoom(struct Room*);
void DisplayMenu();
void GetNewestDir(char*);
void ReadFiles(char*, struct Room*);
void FileToStruct(char*, struct Room*, int);
struct Room* Menu(struct Room*, struct Room*);
int IsConnection(char*, struct Room*);
struct Room* FindRoom(char*, struct Room*);

int main() {
	struct Room rooms[NUM_ROOMS];
	int stepsTaken = 0;
	struct Room path[100];
	// set all Rooms numConnections to 0
	int i;
	for(i = 0; i < NUM_ROOMS; i++) {
		rooms[i].numConnections = 0;
	}

	char newestDirName[256]; // Holds the name of the newest dir that contains prefix
	DisplayMenu();
	GetNewestDir(newestDirName);
	ReadFiles(newestDirName, rooms);
	
	

	struct Room* currentRoom = FindStartRoom(rooms);

	do {
		currentRoom = Menu(currentRoom, rooms);

	} while(!IsEndRoom(currentRoom));

	return 0;
}

// Checks to see if a given string is a room name in room's connections
int IsConnection(char* str, struct Room* room) {
	int isConnection = 0;
	int i;
	for(i = 0; i < room->numConnections; i++) {
		if(strcmp(str, room->connections[i]) == 0) {
			isConnection = 1;
		}
	}
	return isConnection;
}

// Finds a room in array of rooms
struct Room* FindRoom(char* room, struct Room* rooms) {
	struct Room* newRoom = NULL;
	int i;
	for(i = 0; i < NUM_ROOMS; i++) {
		if(strcmp(room, rooms[i].name) == 0) {
			newRoom = &rooms[i];
		}
	}
	return newRoom;
}

// Displays the menu
struct Room* Menu(struct Room* room, struct Room* rooms) {
	printf("CURRENT LOCATION: %s\n", room->name);
	printf("POSSIBLE CONNECTIONS: ");
	int i;
	for(i = 0; i < room->numConnections; i++) {
		if(i == (room->numConnections - 1))
			printf("%s.\n", room->connections[i]);
		else
			printf("%s, ", room->connections[i]);
	}
	printf("WHERE TO? >");
	char* buffer;
	size_t buffsize = 10;
	size_t characters;

	buffer = (char*)malloc(buffsize * sizeof(char));
	characters = getline(&buffer, &buffsize, stdin);
	// strip the newline character from input buffer so a match can be made to room names
	buffer[characters - 1] = '\0';
	if(IsConnection(buffer, room)) {
		room = FindRoom(buffer, rooms);
		printf("New room is %s\n", room->name);
		return room;
	}
	else {
		printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
	}
	free(buffer);
}

// IsEndRoom is a bool function that checks to see if a room is the end room
int IsEndRoom(struct Room* room) {
	int isEnd = 0;
	if(strcmp(room->type, "END_ROOM") == 0) {
		isEnd = 1;
	}
	return isEnd;
}


// FindStartRoom loops through the rooms array to find the start room
struct Room* FindStartRoom(struct Room* rooms) {
	int i;
	for(i = 0; i < NUM_ROOMS; i++) {
		if(strcmp(rooms[i].type, "START_ROOM") == 0) {
			return &rooms[i];
		}
	}
}

void DisplayMenu() {
	printf("CURRENT LOCATION: %s\n", "TBD");
	printf("POSSIBLE CONNECTIONS: \n");
	printf("WHERE TO? >");
}

void GetNewestDir(char* newestDirName) {
	// The following code was taken from 2.4 Notes; thanks Professor
	int newestDirTime = -1; // Modified timestamp of newest subdir examined
	char targetDirPrefix[32] = "harvellk.rooms."; // Prefix we're looking for
	memset(newestDirName, '\0', sizeof(newestDirName));

	DIR* dirToCheck; // Holds the directory we're starting in
	struct dirent* fileInDir; // Holds the current subdir of the starting dir
	struct stat dirAttributes; // Holds information we've gained about subdir

	dirToCheck = opendir("."); // Open up the directory this program was run in

	if (dirToCheck > 0 ) // Make sure the current directory could be opened
	{
		while (( fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
		{
			if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
			{
				printf("Found the prefix: %s\n", fileInDir->d_name);
				stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

				if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
				{
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, "./");
					strcat(newestDirName, fileInDir->d_name);
					printf("Newer subdir: %s, new time: %d\n",
							newestDirName, newestDirTime);
				}
			}
		}
	}
	closedir(dirToCheck); // Close the directory we opened
}


// ReadFiles reads the Room files in the Rooms directory and sends them to FileToStruct to be processed
void ReadFiles(char* dirStr, struct Room* rooms) {
	struct dirent *fileInDir;
	DIR* dir = opendir(dirStr); // Open up the directory to read files
	if (dir > 0) // Make sure the directory can be opened
	{
		int roomNum = 0; // Helps keep track of which room in the struct array
		// go through each file in the rooms folder
		while ((fileInDir = readdir(dir)) != NULL)
		{
			// make sure the file isn't the "." or ".." directories
			if((strcmp(fileInDir->d_name, ".") != 0) && strcmp(fileInDir->d_name, "..") != 0)
			{

				printf("%s\n", fileInDir->d_name);
				// create a file path string to open the file and process contents
				char filePath[35];
				strcpy(filePath, dirStr);
				strcat(filePath, "/");
				strcat(filePath, fileInDir->d_name);
				// send to FileToStruct function to convert file into Room struct
				FileToStruct(filePath, rooms, roomNum);
				roomNum++;
			}
		}
		closedir(dir);
	}
}

// Processes received file paths to convert file contents into Room Struct placed in Rooms array.
void FileToStruct(char* filePath, struct Room* rooms, int roomNum) {
	// open the file
	FILE *fptr = fopen(filePath, "r");
	// make sure the file exists
	if (fptr != NULL) {
		char line[100];
		// get each line one at a time
		while(fgets(line, sizeof line, fptr) != NULL) {
			//fputs(line, stdout);
			// check if the line is a ROOM NAME line
			if(strstr(line, "ROOM NAME")) {
				//fputs(line, stdout);
				// clear out the rooms name
				memset(rooms[roomNum].name, 0, MAX_NAME_SIZE);
				// transfer line contents after "ROOM NAME: " (11 characters) into name var
				memcpy(rooms[roomNum].name, &line[11], MAX_NAME_SIZE);
				// strip the '\n' at the end of the line
				rooms[roomNum].name[strcspn(rooms[roomNum].name, "\n")] = 0;
				// terminate the string with a null terminator just in case
				rooms[roomNum].name[MAX_NAME_SIZE] = '\0';
			} 
			// check if the line is a CONNECTION line
			else if(strstr(line, "CONNECTION")) {
				// placeholder for connections array to enhance readability
				int temp = rooms[roomNum].numConnections;
				// clear out the connection name
				memset(rooms[roomNum].connections[temp], 0, MAX_NAME_SIZE);
				// transfer line contents after "CONNECTION #: " (14 characters) into connections
				memcpy(rooms[roomNum].connections[temp], &line[14], MAX_NAME_SIZE);
				// strip the '\n' at the end of the line
				rooms[roomNum].connections[temp][strcspn(rooms[roomNum].connections[temp], "\n")] = 0;
				// terminate the string with a null terminator just in case
				rooms[roomNum].connections[temp][MAX_NAME_SIZE] = '\0';
				// increment the numConnections variable
				rooms[roomNum].numConnections++;
			}
			// check if the line is a ROOM TYPE line
			else if(strstr(line, "ROOM TYPE")) {
				// clear out the room type name
				memset(rooms[roomNum].type, 0, 11);
				// transfer line contents after "ROOM TYPE: " (11 characters) into type
				memcpy(rooms[roomNum].type, &line[11], 11);
				rooms[roomNum].type[10] = '\0';
			}
		}
		fclose(fptr);
	}
	else {
		perror(filePath);
	}
}
