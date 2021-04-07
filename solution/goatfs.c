#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "disk.h"
#include "goatfs.h"

int FREE_MAP = 0;
int DATA_MAP = 0;

Block sBlock;
Block block;
unsigned int inodeCount;

void debug()
{	
	// Start by reading the superblock
	wread(0, sBlock.Data);

	printf("SuperBlock:\n");

	if (sBlock.Super.MagicNumber == MAGIC_NUMBER)
	{
		printf("    magic number is valid\n");
	}
	else
	{
		printf("    magic number is invalid\n");
	}
	
	// spacing is weird because tab set to 4 on my local, and 5 on server
	printf("    %u blocks\n", sBlock.Super.Blocks);
	printf("    %u inode blocks\n", sBlock.Super.InodeBlocks);
	printf("    %u inodes\n", sBlock.Super.Inodes);
	
	unsigned int inodeCount = sBlock.Super.InodeBlocks;
	int indirectInt = 0;
	char *direct = NULL;
	char *indirect = NULL;

	for (unsigned int y=0; y<inodeCount; y++){
		wread(1+y, sBlock.Data);
		// looping through individual inodes numbering 
		// them by their pointer number, data in second column
		for (unsigned int e=0; e<INODES_PER_BLOCK; e++){
			if (!sBlock.Inodes[e].Valid) {
                continue;
            }
			for (unsigned int l=0; l < POINTERS_PER_INODE; l++){
				if (sBlock.Inodes[e].Direct[l] != 0){
					direct += ' ';
					direct += sBlock.Inodes[e].Direct[l];
				}
			}
			// indirect = sBlock.Inodes[y].Indirect;
			if (indirectInt != 0){
				wread(indirectInt, block.Data);
				for (int j=0; j < POINTERS_PER_BLOCK; j++){
					if (block.Pointers[j] != 0){
						indirect += ' ';
						indirect += block.Pointers[j];
					}
				}
			}
			printf("Inode %u:\n", e);
			printf("    size: %u bytes\n", block.Inodes[y].Size);
			printf("    direct blocks: %c\n", *direct);
			if (indirect != NULL){
				printf("    indirect blocks: %c\n", *indirect);
			}
		}
	}
}

bool format()
{
	return true;
}

int mount()
{
	// examine the disk for a filesystem	
	if (block.Super.MagicNumber != MAGIC_NUMBER){
		wread(0, block.Data); // read superblock

		return true;
	}
	else{
		return false;
	}
	if (_disk->Blocks != 0)
	{
		// wread();
		// read superblock
		// build free block bitmap
		// prepare filesystem for use
		return true;
	}
}

ssize_t create()
{
	Inode inode;
	inode.Size = 0;

	if (inode.Valid)
	{
		return true;
	}
	else
	{
		return -1;
	}
}

bool wremove(size_t inumber)
{
	return true;
}

ssize_t stat(size_t inumber)
{
	return 4;
}

ssize_t wfsread(size_t inumber, char *data, size_t length, size_t offset)
{
	return 4;
}

ssize_t wfswrite(size_t inumber, char *data, size_t length, size_t offset)
{
	return 4;
}
