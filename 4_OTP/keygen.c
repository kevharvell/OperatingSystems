#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	srand(time(NULL));
	// check if correct # of arguments
	if(argc < 2) {
		fprintf(stderr, "Not enough arguments. \n");
		exit(1);
	}

	// create the key using provided length from arguments
	char* key;
	int length = atoi(argv[1]);
	int i;

	// allocate memory for the key using provided argument
	key = malloc((length + 1) * sizeof(char));
	memset(key, '\0', sizeof(key));

	// create random letters A-Z and @ ranging from char codes 64-90.
	// 64 is actually '@', which we don't need but we do need a space
	// Thus, anytime '@' is encountered, turn it into a space
	char letter;
	for(i = 0; i < length; i++) {
		letter = rand() % 27 + 64;
		if(letter == '@')
			letter = ' ';
		printf("%c", letter);
	}
	printf("\n");

	free(key);
	return 0;
}
