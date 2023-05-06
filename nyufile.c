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
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned int u32;

const u32 STARTING_CLUSTER = 2;
const u32 DIR_ENTRY_SIZE = 32;

#pragma pack(push, 1)
typedef struct BootEntry
{
	unsigned char BS_jmpBoot[3];     // Assembly instruction to jump to boot code
	unsigned char BS_OEMName[8];     // OEM Name in ASCII
	unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
	unsigned char BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
	unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
	unsigned char BPB_NumFATs;       // Number of FATs
	unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
	unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
	unsigned char BPB_Media;         // Media type
	unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
	unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
	unsigned short BPB_NumHeads;      // Number of heads in storage device
	u32 BPB_HiddSec;       // Number of sectors before the start of partition
	u32 BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
	u32 BPB_FATSz32;       // 32-bit size in sectors of one FAT
	unsigned short BPB_ExtFlags;      // A flag for FAT
	unsigned short BPB_FSVer;         // The major and minor version number
	u32 BPB_RootClus;      // Cluster where the root directory can be found
	unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
	unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
	unsigned char BPB_Reserved[12];  // Reserved
	unsigned char BS_DrvNum;         // BIOS INT13h drive number
	unsigned char BS_Reserved1;      // Not used
	unsigned char BS_BootSig;        // Extended boot signature to identify if the next three values are valid
	u32 BS_VolID;          // Volume serial number
	unsigned char BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
	unsigned char BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct DirEntry
{
	unsigned char DIR_Name[11];      // File name
	unsigned char DIR_Attr;          // File attributes
	unsigned char DIR_NTRes;         // Reserved
	unsigned char DIR_CrtTimeTenth;  // Created time (tenths of second)
	unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
	unsigned short DIR_CrtDate;       // Created day
	unsigned short DIR_LstAccDate;    // Accessed day
	unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
	unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
	unsigned short DIR_WrtDate;       // Written day
	unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
	u32 DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct Offsets {
	u32 FAT_area_size;
	u32 reserved_sectors_size;
	u32 root_dir_start;
	u32 cluster_size;
	u32 root_entry_size;
	u32 data_area_start;
}Offsets;
#pragma pack(pop)

void print_usage_information()
{
	printf("Usage: ./nyufile disk <options>\n");
	printf("  -i                     Print the file system information.\n");
	printf("  -l                     List the root directory.\n");
	printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
	printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

BootEntry get_file_system_info(int fd)
{
	struct stat buffer;
	if (fstat(fd, &buffer) == -1)
	{
		perror("fstat error");
		exit(EXIT_FAILURE);
	}

	void *data = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
	{
		perror("mmap error");
		exit(EXIT_FAILURE);
	}

	BootEntry boot_entry = *((BootEntry *) data);
	return boot_entry;
}

void print_file_system_info(BootEntry boot_entry)
{
	printf("Number of FATs = %u\n", boot_entry.BPB_NumFATs);
	printf("Number of bytes per sector = %u\n", boot_entry.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %u\n", boot_entry.BPB_SecPerClus);
	printf("Number of reserved sectors = %u\n", boot_entry.BPB_RsvdSecCnt);
}

char *convert_file_name(char *file_name)
{
	int dot_index = 8;
	char extension[3];

	for (int i = 0; i < 8; i++) // extract the file name (without extension)
	{
		if (file_name[i] == ' ')
		{
			dot_index = i;
			break;
		}
	}
	for (int i = 0; i < 3; i++) // extract the extension
	{
		extension[i] = file_name[8 + i];
	}

	if (extension[0] == ' ')
	{
		file_name[dot_index] = '\0';
	}
	else // put '.' and extension right after file name
	{
		file_name[dot_index] = '.';
		for (int i = 0; i < 3; i++)
		{
			if (extension[i] == ' ')
			{
				file_name[dot_index + 1 + i] = '\0';
				break;
			}
			file_name[dot_index + 1 + i] = extension[i];
		}
		file_name[dot_index + 4] = '\0';
	}
	return file_name;
}

bool same_file_name_except_first(char *str1, char *str2)
{
	for (int i = 1; i < (int) strlen(str1); i++)
	{
		if (str1[i] != str2[i])
		{
			return false;
		}
	}
	return true;
}

Offsets calculate_offsets(BootEntry boot_entry)
{
	Offsets offsets;

	offsets.FAT_area_size = boot_entry.BPB_NumFATs * boot_entry.BPB_FATSz32 * boot_entry.BPB_BytsPerSec;
	offsets.reserved_sectors_size = boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytsPerSec;
	offsets.root_dir_start = offsets.reserved_sectors_size + offsets.FAT_area_size;
	offsets.cluster_size = boot_entry.BPB_SecPerClus * boot_entry.BPB_BytsPerSec;
	offsets.root_entry_size = boot_entry.BPB_RootEntCnt * DIR_ENTRY_SIZE;
	offsets.data_area_start = offsets.root_dir_start + offsets.root_entry_size;

	return offsets;
}

void traverse_root_directory(int fd, BootEntry boot_entry, char flag, char *file_to_recover)
{
	Offsets offsets = calculate_offsets(boot_entry);
	u32 current_cluster = boot_entry.BPB_RootClus;

	int entry_count = 0;
	int same_file_count = 0;
	bool root_dir_end = false;

	DirEntry entry_to_recover;
	off_t entry_to_recover_position;

	while (current_cluster != 0x0FFFFFFF && !root_dir_end) // End of the cluster chain
	{
		u32 cluster_offset = (current_cluster - 2) * offsets.cluster_size;
		lseek(fd, offsets.data_area_start + cluster_offset, SEEK_SET);

		DirEntry dir_entry;
		off_t dir_entry_position = lseek(fd, 0, SEEK_CUR);

		for (u32 i = 0; i < offsets.cluster_size; i += DIR_ENTRY_SIZE)
		{
			if (read(fd, &dir_entry, DIR_ENTRY_SIZE) != DIR_ENTRY_SIZE) // read current directory entry
			{
				root_dir_end = true; // if we can't read a full entry size, it's the end of directory
				break;
			}

			if (dir_entry.DIR_Name[0] == 0x00) // 0x00 -> End of root dir
			{
				root_dir_end = true;
				break;
			}

			char *file_name = (char *) malloc(13 * sizeof(char));
			memcpy(file_name, dir_entry.DIR_Name, 11);
			file_name = convert_file_name(file_name);

			// List root directory
			if (dir_entry.DIR_Name[0] != 0xE5 && flag == 'l') // skip deleted files
			{
				if (dir_entry.DIR_Attr & 0x10) // entry is a directory
				{
					printf("%s/ (starting cluster = %u)\n", file_name, dir_entry.DIR_FstClusLO);
				}
				else
				{
					if (dir_entry.DIR_FileSize == 0)
						printf("%s (size = %u)\n", file_name, dir_entry.DIR_FileSize);
					else
						printf("%s (size = %u, starting cluster = %u)\n", file_name, dir_entry.DIR_FileSize,
							   dir_entry.DIR_FstClusLO);
				}
				entry_count++;
				continue;
			}

			// Detect deleted files
			if (dir_entry.DIR_Name[0] == 0xE5 && flag == 'r')
			{
				if (same_file_name_except_first(file_name, file_to_recover))
				{
					same_file_count++;
					entry_to_recover = dir_entry;
					entry_to_recover_position = dir_entry_position;
				}

			}
		}

		//Read next cluster from FAT
		lseek(fd, offsets.reserved_sectors_size + (current_cluster * 4), SEEK_SET);
		read(fd, &current_cluster, 4);
	}

	//recover small contiguous file
	if (flag == 'r' && same_file_count == 1)
	{
		//change directory name
		entry_to_recover.DIR_Name[0] = file_to_recover[0];
		lseek(fd, entry_to_recover_position, SEEK_SET);
		write(fd, &entry_to_recover, DIR_ENTRY_SIZE);

		//update the fat info
		u32 next_cluster = 0x0FFFFFFF; // to mark end of cluster chain
		u32 fat_offset = offsets.reserved_sectors_size + (entry_to_recover.DIR_FstClusLO * 4);
		lseek(fd, fat_offset, SEEK_SET);
		write(fd, &next_cluster, 4);

		printf("%s: successfully recovered\n", file_to_recover);
	}
	else if (flag == 'r' && same_file_count == 0)
	{
		printf("%s: file not found\n", file_to_recover);

	}
	else if (flag == 'r')
	{
		printf("%s: multiple candidates found\n", file_to_recover);
	}

	if (flag == 'l')
	{
		printf("Total number of entries = %d\n", entry_count);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		print_usage_information();
		return 1;
	}

	char *disk_image_arg = argv[1];

	int ch;
	while ((ch = getopt(argc, argv, "ilr:R:")) != -1)
	{
		switch (ch)
		{
			case 'i':
			{
				int fd = open(disk_image_arg, O_RDWR);
				if (fd == -1)
				{
					perror("Error opening the disk image.");
					return 1;
				}
				BootEntry boot_entry = get_file_system_info(fd);
				print_file_system_info(boot_entry);
				close(fd);
				break;
			}
			case 'l':
			{
				int fd = open(disk_image_arg, O_RDWR);
				if (fd == -1)
				{
					perror("Error opening the disk image.");
					return 1;
				}
				BootEntry boot_entry = get_file_system_info(fd);
				traverse_root_directory(fd, boot_entry, 'l', NULL);
				close(fd);
				break;
			}
			case 'r':
			{
				// Recover a contiguous file
				int fd = open(disk_image_arg, O_RDWR);
				if (fd == -1)
				{
					perror("Error opening the disk image.");
					return 1;
				}
				BootEntry boot_entry = get_file_system_info(fd);
				traverse_root_directory(fd, boot_entry, 'r', argv[3]);
				break;
			}
			case 'R':
			{
				// Recover a possibly non-contiguous file
			}
			default :
			{
				print_usage_information();
				return 1;
			}
		}
	}

	return 0;
}




