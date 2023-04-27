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

typedef struct {
	unsigned int num_FATs;
	unsigned int bytes_per_sector;
	unsigned int sectors_per_cluster;
	unsigned int reserved_sectors;
} FAT32Info;

void print_usage_information()
{
	printf("Usage: ./nyufile disk <options>\n");
	printf("  -i                     Print the file system information.\n");
	printf("  -l                     List the root directory.\n");
	printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
	printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

void print_file_system_information(FILE *disk_image)
{
	unsigned char boot_sector[512];
	fread(boot_sector, 1, 512, disk_image);

	FAT32Info info;

	const int NUM_FATS_OFFSET = 16;
	const int BYTES_PER_SECTOR_OFFSET = 11;
	const int SECTORS_PER_CLUSTER_OFFSET = 13;
	const int RESERVED_SECTORS_OFFSET = 14;

	info.num_FATs = boot_sector[NUM_FATS_OFFSET];
	info.bytes_per_sector = boot_sector[BYTES_PER_SECTOR_OFFSET] | (boot_sector[BYTES_PER_SECTOR_OFFSET + 1] << 8);
	info.sectors_per_cluster = boot_sector[SECTORS_PER_CLUSTER_OFFSET];
	info.reserved_sectors = boot_sector[RESERVED_SECTORS_OFFSET] | (boot_sector[RESERVED_SECTORS_OFFSET + 1] << 8);


	printf("Number of FATs = %u\n", info.num_FATs);
	printf("Number of bytes per sector = %u\n", info.bytes_per_sector);
	printf("Number of sectors per cluster = %u\n", info.sectors_per_cluster);
	printf("Number of reserved sectors = %u\n", info.reserved_sectors);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		print_usage_information();
		return 1;
	}

	char *disk_image_arg = argv[1];
	char *option = argv[2];

	if (strcmp(option, "-i") == 0)
	{
		if (argc != 3)
		{
			print_usage_information();
			return 1;
		}

		FILE *disk_image = fopen(disk_image_arg, "rb");
		if (disk_image == NULL)
		{
			perror("Error opening the disk image.");
			return 1;
		}
		print_file_system_information(disk_image);
		fclose(disk_image);
	}
	else if (strcmp(option, "-l") == 0)
	{
		if (argc != 3)
		{
			print_usage_information();
			return 1;
		}

		// List the root directory
	}
	else if (strcmp(option, "-r") == 0)
	{
		if (argc != 4 && !(argc == 6 && strcmp(argv[4], "-s") == 0))
		{
			print_usage_information();
			return 1;
		}

		//char *filename = argv[3];
		// Recover a contiguous file
	}
	else if (strcmp(option, "-R") == 0)
	{
		if (!(argc == 6 && strcmp(argv[4], "-s") == 0))
		{
			print_usage_information();
			return 1;
		}

		//char *filename = argv[3];
		// Recover a possibly non-contiguous file
	}
	else
	{
		print_usage_information();
		return 1;
	}

	return 0;
}


