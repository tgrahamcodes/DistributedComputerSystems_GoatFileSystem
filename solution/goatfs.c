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
Block new;
unsigned int inodeCount;
unsigned int blockCount;

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
	char *direct = NULL;
	char *indirect = NULL;

	for (unsigned int y=0; y<inodeCount; y++){
		wread(1+y, sBlock.Data);
		// looping through individual inodes numbering 
		// them by their pointer number, data in second column
		for (unsigned int e=0; e<INODES_PER_BLOCK; e++){
			// printf("%u", block.Inodes[e].Size);
			if (!sBlock.Inodes[e].Valid) {
				// printf("%u ", sBlock.Inodes[e].Valid);
                continue;
            }
			for (unsigned int l=0; l < POINTERS_PER_INODE; l++){
				if (sBlock.Inodes[e].Direct[l] != 0){
					direct += ' ';
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
				// printf("%u ", sBlock.Inodes->Valid);
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
	
	if (_disk->Mounts > 0){
		return 1;
	}
	
	wread(0,new.Data);

	if (new.Super.MagicNumber != MAGIC_NUMBER){
		return 1;
	}

	if (new.Super.Blocks < 0){
		return 1;
	}

	if (new.Super.Inodes != (new.Super.InodeBlocks * INODES_PER_BLOCK)){
		return 1;
	}
	if (new.Super.InodeBlocks != ceil(.1 * new.Super.Blocks)) {
        return 1;
    } 

	memset(FREE_MAP,0,BLOCK_SIZE);
	FREE_MAP[0] = 0;
	// printf("FREE_MAP = %u \n", (unsigned int)sizeof(FREE_MAP));
	for (unsigned int i=1; i <= new.Super.Blocks; i++){
		FREE_MAP[i] = 1;
	}

	for (unsigned int j=0; j < new.Super.InodeBlocks; j++){
		FREE_MAP[j+1] = 0;
	}
	
	blockCount = new.Super.Blocks;

	for (unsigned int x=0; x < new.Super.InodeBlocks; x++){
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

void initialize_inode(Inode *node) {
    for (uint32_t i = 0; i < POINTERS_PER_INODE; i++) {
        node->Direct[i] = 0;
    }
    node->Indirect  = 0;
    node->Size      = 0;
}

ssize_t create() {
    // Locate free inode in inode table
    ssize_t inodeNumber = -1;
    for (uint32_t i = 0; i < INODES_PER_BLOCK; i++) {
		uint32_t x = 0;
        Block inodeBlock;
        wread(x+1, inodeBlock.Data);
        for (uint32_t j = 0; j < INODES_PER_BLOCK + 1; j++){
            if (!inodeBlock.Inodes[j].Valid){
                inodeBlock.Inodes[j].Valid = 1;
                inodeNumber = j+INODES_PER_BLOCK*i;
				if (inodeNumber > 128){
					inodeNumber = -1;
					break;
				}
                initialize_inode(&inodeBlock.Inodes[j]);
                wwrite(x+1, inodeBlock.Data);
                break;
            }
        }
        if (inodeNumber != -1) {
            break;
        }
		x++;
	}

    return inodeNumber;
}

bool wremove(size_t inumber)
{
	Inode inode;

    if (!loadInode(inumber, &inode)) {
        return false;
    }
    if (inode.Valid == 0) {
        return false;
    }

    for (unsigned int i = 0; i < POINTERS_PER_INODE; i++) {
        if (inode.Direct[i] != 0) {
            FREE_MAP[inode.Direct[i]] = 1;
            inode.Direct[i] = 0;
        }
    }

    if (inode.Indirect != 0) {
        FREE_MAP[inode.Indirect] = 1;
        wread(inode.Indirect, block.Data);
        for (unsigned int i = 0; i < POINTERS_PER_BLOCK; i++) {
            if (block.Pointers[i] != 0) {
                FREE_MAP[block.Pointers[i]] = 1;
            }
        }
    }
   	inode.Indirect = 0;
    inode.Valid = 0;
    inode.Size = 0;
    if (!saveInode(inumber, &inode)) {
        return false;
    };

    return true;
}

ssize_t stat(size_t inumber)
{
	Inode inode;
    if (!loadInode(inumber,&inode) || !inode.Valid) {
        return -1;
    }

    return inode.Size;
}

ssize_t wfsread(size_t inumber, char *data, size_t length, size_t offset)
{
	return 4;
}

ssize_t wfswrite(size_t inumber, char *data, size_t length, size_t offset)
{
	return 4;
}

bool loadInode(ssize_t inumber, Inode *inode) {
    size_t blockNum = 1 + (inumber / INODES_PER_BLOCK);
    size_t offset = inumber % INODES_PER_BLOCK;

    if (inumber >= sBlock.Super.Inodes) {
        return false;
    }

    Block block;
    wread(blockNum, block.Data);

    *inode = block.Inodes[offset];

    return true;
}

bool saveInode(ssize_t inumber, Inode *inode) {

    size_t blockNum = 1 + inumber / INODES_PER_BLOCK;
    size_t offset = inumber % INODES_PER_BLOCK;

    if (inumber >= sBlock.Super.Inodes) {
        return false;
    }
	Block b;
    wread(blockNum, b.Data);
	printf("%u", inode->Valid);
    b.Inodes[offset] = *inode;
    wwrite(blockNum, b.Data);

    return true;
}
