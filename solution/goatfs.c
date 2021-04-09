#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "disk.h"
#include "goatfs.h"

char FREE_MAP[BLOCK_SIZE];
char DATA_MAP[BLOCK_SIZE];

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
	}else
	{
		printf("    magic number is invalid\n");
	}
	
	// spacing is weird because tab set to 4 on my local, and 5 on server
	printf("    %u blocks\n", sBlock.Super.Blocks);
	printf("    %u inode blocks\n", sBlock.Super.InodeBlocks);
	printf("    %u inodes\n", sBlock.Super.Inodes);
	
	unsigned int inodeCount = sBlock.Super.InodeBlocks;
	unsigned int indirectInt = 0;
	char *direct;
	char *indirect;

	for (unsigned int y=0; y<inodeCount; y++){
		wread(1+y, sBlock.Data);
		// looping through individual inodes numbering 
		// them by their pointer number, data in second column
		for (unsigned int e=0; e<INODES_PER_BLOCK; e++){
			// printf("%u", block.Inodes[e].Size);
			if (!sBlock.Inodes[e].Valid) {
                continue;
            }
			for (unsigned int l=0; l < POINTERS_PER_INODE; l++){
				if (sBlock.Inodes[e].Direct[l] != 0){
					direct = direct + ' ';
					direct += sBlock.Inodes[e].Direct[l];
				}
			}
			indirectInt = sBlock.Inodes[e].Indirect;
			if (indirectInt != 0){
				wread(indirectInt, block.Data);
				for (unsigned int j=0; j < POINTERS_PER_BLOCK; j++){
					if (block.Pointers[j] != 0){
						indirect += ' ';
						indirect += block.Pointers[j];
					}
				}
			}
			printf("Inode %u:\n", e);
			printf("    size: %u bytes\n", sBlock.Inodes[e].Size);
			if (sBlock.Inodes[e].Valid != 0){
				printf("    direct blocks:");
				for (unsigned int p=0; p < POINTERS_PER_INODE; p++){
					if (sBlock.Inodes[e].Direct[p] != 0){
						printf(" %u", sBlock.Inodes[e].Direct[p]);
					}
				}
				for (unsigned int p=0; p <= sBlock.Inodes->Size; p++){
					if (sBlock.Inodes[e].Indirect != 0){
						printf("\n    indirect block: %u\n", indirectInt);
						printf("    indirect data blocks:");
						for (unsigned int m=0; m <= 300; m++){
							if (block.Pointers[m] != 0){
								printf(" %u", block.Pointers[m]);
							}
						}
					} 
				}
			}
			printf("\n");
		}
	}
}



bool format()
{
	if (_disk->Mounts > 0){
		return false;
	}

	// printf("File Descriptor: %u", _disk->FileDescriptor);
    memset(sBlock.Data,0,BLOCK_SIZE);
	sBlock.Super.MagicNumber = MAGIC_NUMBER;
    sBlock.Super.Blocks = _disk->Blocks;
    sBlock.Super.InodeBlocks = (size_t)(((float)_disk->Blocks*0.1)+0.5);
    sBlock.Super.Inodes = INODES_PER_BLOCK*sBlock.Super.InodeBlocks;
   	wwrite(0, sBlock.Data);

  
    char clear[BUFSIZ] = {0};
    for (size_t i=1; i<sBlock.Super.Blocks; i++) {
        wwrite(i, clear);
    }

	return true;
}

int mount()
{
	Block new;
	if (_disk->Mounts > 0){
		return 1;
	}

	wread(0,sBlock.Data);

	if (sBlock.Super.MagicNumber != MAGIC_NUMBER){
		return 1;
	}

	if (sBlock.Super.Blocks < 0){
		return 1;
	}

	if (sBlock.Super.Inodes != (sBlock.Super.InodeBlocks * INODES_PER_BLOCK)){
		return 1;
	}
	if (sBlock.Super.InodeBlocks != ceil(.1 * sBlock.Super.Blocks)) {
        return 1;
    } 

	memset(FREE_MAP,0,BLOCK_SIZE);
	FREE_MAP[0] = 0;
	// printf("FREE_MAP = %u \n", (unsigned int)sizeof(FREE_MAP));
	for (unsigned int i=1; i <= sBlock.Super.Blocks; i++){
		FREE_MAP[i] = 1;
	}

	for (unsigned int j=0; j < sBlock.Super.InodeBlocks; j++){
		FREE_MAP[j+1] = 0;
	}
	
	for (unsigned int x=0; x < sBlock.Super.InodeBlocks; x++){
		wread(x+1, new.Data);
		
		for (unsigned int n=0; n < INODES_PER_BLOCK; n++){
			if(!new.Inodes[n].Valid){
				continue;
			}
			unsigned int b = ceil(new.Inodes[n].Size/(double)BLOCK_SIZE);
			
			for (unsigned int e=0; e < POINTERS_PER_INODE && e < b; e++){
				FREE_MAP[new.Inodes[n].Direct[e]] = 0;
			}

			if (b > POINTERS_PER_INODE){
				Block ind;
				wread(new.Inodes[n].Indirect, ind.Data);
				FREE_MAP[new.Inodes[n].Indirect] = 0;
				for (unsigned int l = 0; l < b - POINTERS_PER_INODE; l++){
					FREE_MAP[ind.Pointers[l]] = 0;
				}
			}
		}
	}
	_disk->Mounts++;
	return 0;
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
