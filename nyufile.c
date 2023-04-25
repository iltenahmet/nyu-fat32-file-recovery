/*
#nyufile
To use valgrind to check for memory leaks and track origins:
valgrind --leak-check=full --track-origins=yes ./nyufile
To run a Docker container on WSL:
docker run -i --name cs202 --privileged --rm -t -v /mnt/c/users/ailte/os:/cs202 -w /cs202 ytang/os bash
To run a Docker container on Mac:
docker run -i --name cs202 --privileged --rm -t -v /Users/ahmetilten/OS/labs:/cs202 -w /cs202 ytang/os bash
To zip specific files:
zip nyufile.zip Makefile *.h *.c
 */

#include <string.h>
#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void print_usage_information() {
	printf("Usage: ./nyufile disk <options>\n");
	printf("  -i                     Print the file system information.\n");
	printf("  -l                     List the root directory.\n");
	printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
	printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		print_usage_information();
		return 1;
	}

	//char *disk_image = argv[1];
	char *option = argv[2];

	if (strcmp(option, "-i") == 0) {
		if (argc == 3) {
			// Print the file system information
		} else {
			print_usage_information();
			return 1;
		}
	} else if (strcmp(option, "-l") == 0) {
		if (argc == 3) {
			// List the root directory
		} else {
			print_usage_information();
			return 1;
		}
	} else if (strcmp(option, "-r") == 0) {
		if (argc == 4 || (argc == 6 && strcmp(argv[4], "-s") == 0)) {
			//char *filename = argv[3];
			// Recover a contiguous file
		} else {
			print_usage_information();
			return 1;
		}
	} else if (strcmp(option, "-R") == 0) {
		if (argc == 6 && strcmp(argv[4], "-s") == 0) {
			//char *filename = argv[3];
			// Recover a possibly non-contiguous file
		} else {
			print_usage_information();
			return 1;
		}
	} else {
		print_usage_information();
		return 1;
	}

	return 0;
}

