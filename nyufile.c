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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const int STARTING_CLUSTER = 2;
const size_t DIR_ENTRY_SIZE = 32;

#pragma pack(push,1)
typedef struct BootEntry {
	unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
	unsigned char  BS_OEMName[8];     // OEM Name in ASCII
	unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
	unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
	unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
	unsigned char  BPB_NumFATs;       // Number of FATs
	unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
	unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
	unsigned char  BPB_Media;         // Media type
	unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
	unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
	unsigned short BPB_NumHeads;      // Number of heads in storage device
	unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
	unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
	unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
	unsigned short BPB_ExtFlags;      // A flag for FAT
	unsigned short BPB_FSVer;         // The major and minor version number
	unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
	unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
	unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
	unsigned char  BPB_Reserved[12];  // Reserved
	unsigned char  BS_DrvNum;         // BIOS INT13h drive number
	unsigned char  BS_Reserved1;      // Not used
	unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
	unsigned int   BS_VolID;          // Volume serial number
	unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
	unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct DirEntry {
	unsigned char  DIR_Name[11];      // File name
	unsigned char  DIR_Attr;          // File attributes
	unsigned char  DIR_NTRes;         // Reserved
	unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
	unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
	unsigned short DIR_CrtDate;       // Created day
	unsigned short DIR_LstAccDate;    // Accessed day
	unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
	unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
	unsigned short DIR_WrtDate;       // Written day
	unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
	unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

void print_usage_information()
{
	printf("Usage: ./nyufile disk <options>\n");
	printf("  -i                     Print the file system information.\n");
	printf("  -l                     List the root directory.\n");
	printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
	printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

BootEntry get_file_system_information(int fd)
{
	struct stat sb;
	if (fstat(fd, &sb) == -1)
	{
		perror("fstat error");
		exit(EXIT_FAILURE);
	}

	void *mapped_data = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapped_data == MAP_FAILED)
	{
		perror("mmap error");
		exit(EXIT_FAILURE);
	}

	BootEntry boot_entry = *((BootEntry *)mapped_data);
	return boot_entry;
}

void print_file_system_information(BootEntry boot_entry)
{
	printf("Number of FATs = %u\n", boot_entry.BPB_NumFATs);
	printf("Number of bytes per sector = %u\n", boot_entry.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %u\n", boot_entry.BPB_SecPerClus);
	printf("Number of reserved sectors = %u\n", boot_entry.BPB_RsvdSecCnt);
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

		int fd = open(disk_image_arg, O_RDWR);
		if (fd == -1)
		{
			perror("Error opening the disk image.");
			return 1;
		}
		BootEntry boot_entry = get_file_system_information(fd);
		print_file_system_information(boot_entry);
		close(fd);
	}
	else if (strcmp(option, "-l") == 0)
	{
		if (argc != 3)
		{
			print_usage_information();
			return 1;
		}

		// list root directory
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


