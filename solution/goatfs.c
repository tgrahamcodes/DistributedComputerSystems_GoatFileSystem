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

Block block;

void debug()
{
	wread(0, block.Data);

	printf("SuperBlock:\n");

	if (block.Super.MagicNumber == MAGIC_NUMBER)
	{
		printf("    magic number is valid\n");
	}
	else
	{
		printf("    magic number is invalid\n");
	}
	
	// spacing is weird because tab set to 4 on my local, and 5 on server
	printf("    %u blocks\n", block.Super.Blocks);
	printf("    %u inode blocks\n", block.Super.InodeBlocks);
	printf("    %u inodes\n", block.Super.Inodes);
	
	unsigned int iNodeBlocks = block.Super.InodeBlocks;

	wread(1, block.Data);

	// loops through the inode blocks
	//printf("\n");

	int iCounter = 1;
	for (unsigned int y=1; y<=iNodeBlocks; y++){
	printf("Inode %u:\n", iCounter);
		// looping through individual inodes numbering 
		// them by their pointer number, data in second column
		for (unsigned int e=8; e<=128; e=e+1){
			if ((block.Pointers[e] != 0) && (iCounter <= iNodeBlocks)){
				printf("\n    size: %u bytes", block.Pointers[e+1]);
				printf("\n    direct blocks: %u", block.Pointers[e+2]);
			}
		}
		iCounter++;
	}
	printf("\n");
	
	// 1 inode block
	// 128 inodes in that block POINTERS_PER_BLOCK(1024)
	// 5 direct pointers per inode POINTERS_PER_INODE(5)
	// block.Inodes->Valid = block.Pointers[8];
	// block.Inodes->Size = block.Pointers[9];
	// block.Inodes->Direct[POINTERS_PER_INODE] = block.Pointers[10];
	// block.Inodes->Indirect = block.Pointers[11];
	
	// printf("\nInode %d:\n", block.Inodes->Valid);
	// printf("    size: %d bytes", block.Inodes->Size);
	// printf("\n    direct blocks: %d\n", block.Pointers[10]);
	// printf("    indirect blocks: %d\n", block.Inodes->Indirect);
}

bool format()
{
	return true;
}

int mount()
{
	// examine the disk for a filesystem
	if (_disk->Blocks != 0)
	{
		// wread();
		// read superblock
		// build free block bitmap
		// prepare filesystem for use
		return true;
	}
	else
	{
		return false;
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
