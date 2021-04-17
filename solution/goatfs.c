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
unsigned int blockCount;
unsigned int inodeBlocks;

/* A function used to print the status of the disk that is currently
* mounted on the system. Prints to console, no return as void. */ 
void debug()
{	
	// Start by reading the Superblock
	wread(0, sBlock.Data);

    // Print out information about the Superblock
	printf("SuperBlock:\n");
	if (sBlock.Super.MagicNumber == MAGIC_NUMBER)
	{
		printf("    magic number is valid\n");
	}else
	{
		printf("    magic number is invalid\n");
	}
	
	// Tab set to 3 on my local, and 5 on server
	printf("    %u blocks\n", sBlock.Super.Blocks);
	printf("    %u inode blocks\n", sBlock.Super.InodeBlocks);
	printf("    %u inodes\n", sBlock.Super.Inodes);
    
    // Assign variables for the count and "strings".
	unsigned int inodeCount = sBlock.Super.InodeBlocks;
	unsigned int indirectInt = 0;
	char *direct = NULL;
	char *indirect = NULL;

    // Loop through the number of blocks reserved for the inodes
	for (unsigned int y = 0; y  < inodeCount; y++){
		wread(1+y, sBlock.Data);

		// Looping through the individual Inodes
		for (unsigned int e=0; e<INODES_PER_BLOCK; e++){
            // Check that the current Inode is invalid
			if (!sBlock.Inodes[e].Valid) {
                // And continue
                continue;
            }
            unsigned int l = 0;
            // While l less than pointers in each Inode
			while (l < POINTERS_PER_INODE){
                // Check that the Direct pointer is not 0
				if (sBlock.Inodes[e].Direct[l] != 0){
					direct += ' ';
					direct += sBlock.Inodes[e].Direct[l];
				}
                l++;
			}
			indirectInt = sBlock.Inodes[e].Indirect;
            // Check that the indirectInt is not 0
			if (indirectInt != 0){
                // Then read the data segement of the block
				wread(indirectInt, block.Data);
				for (unsigned int j=0; j < POINTERS_PER_BLOCK; j++){
					if (block.Pointers[j] != 0){
						indirect += ' ';
						indirect += block.Pointers[j];
					}
				}
			}
            // Display results to console
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

/* A function used to format the current disk that is mounted
* on the system. */ 
bool format()
{   
    // Setup variables
    double offset = 0.5;
    double multiplier = 0.1;
    char flush [BUFSIZ] = {0};

    // Check if any mounts were done yet, if so ret false.
	if (_disk->Mounts > 0){
		return false;
	}

    // Allocate memory for the sBlock Data segment
    memset(sBlock.Data,0,BLOCK_SIZE);

	sBlock.Super.MagicNumber = MAGIC_NUMBER;
    sBlock.Super.Blocks = _disk->Blocks;

    sBlock.Super.InodeBlocks = (((double) _disk->Blocks *multiplier)+offset);
    sBlock.Super.Inodes = INODES_PER_BLOCK * sBlock.Super.InodeBlocks;

    // Finally write to the 0 block
   	wwrite(0, sBlock.Data);

    // Flush the buffer 
    for (unsigned int i = 1; i < sBlock.Super.Blocks; i++) {
        wwrite(i, flush);
    }

	return true;
}

/* A function used to mount the provided disk block to the system. */ 
int mount()
{
    // Check if there have already been mounts, ret -1 if so
	if (_disk->Mounts != 0){
		return -1;
	}

    // Setup variables
	Block mountSb;
    double offset = 0.1;

    // Read into mountSb.Data
	wread(0, mountSb.Data);

    // Multiple bad disk error checks
	if (mountSb.Super.MagicNumber != MAGIC_NUMBER){
		return -1;
	}
    if (_disk->Blocks != mountSb.Super.Blocks){
        return -1;
    }
    if (mountSb.Super.InodeBlocks != ceil(offset * mountSb.Super.Blocks)) {
        return -1;
    } 
	if (mountSb.Super.Blocks < 0){
		return -1;
	}
	if (mountSb.Super.Inodes != (mountSb.Super.InodeBlocks * INODES_PER_BLOCK)){
		return -1;
	}
	
    // Setup variables for use later in the project
	blockCount = mountSb.Super.Blocks;

	inodeCount = mountSb.Super.Inodes;

	inodeBlocks = mountSb.Super.InodeBlocks;

    // Allocate memory for our FREE_MAP bitmap disk image
	memset(FREE_MAP, 0, BLOCK_SIZE);

    // Set FREE_MAP initally to 1
	for (unsigned int i=0; i < blockCount; i++){
		FREE_MAP[i] = 1;
	}

    // Superblock is 0 
	FREE_MAP[0] = 0;

    // Then set FREE_MAP indices + 1 to 0
	for (unsigned int j=0; j < inodeBlocks; j++){
		FREE_MAP[j+1] = 0;
	}
	
    // Create another block
    Block mountB;

    // Loop through the inode blocks and read incrementing by one each time
	for (unsigned int x = 0; x < inodeBlocks; x++){
		wread(x+1, mountB.Data);
		
        // Loop again through the Inodes per Block
		for (unsigned int n = 0; n < INODES_PER_BLOCK; n++){

            // If current Inode isn't valid continue on
			if(mountB.Inodes[n].Valid == 0){
				continue;
			}
            
            // Set up a variable for conditional checking pointers per node 
			unsigned int b = ceil(mountB.Inodes[n].Size/BLOCK_SIZE);
			
            // Loop through the pointers per Inode setting the FREE_MAP to 0
			for (unsigned int e=  0; e < POINTERS_PER_INODE && e < b; e++){
				FREE_MAP[mountB.Inodes[n].Direct[e]] = 0;
			}

			if (b > POINTERS_PER_INODE){
                // Use another block for the indirect data segment
				Block indB;
				wread(mountB.Inodes[n].Indirect, indB.Data);
				FREE_MAP[mountB.Inodes[n].Indirect] = 0;
				for (unsigned int l = 0; l < b - POINTERS_PER_INODE; l++){
					FREE_MAP[indB.Pointers[l]] = 0;
				}
			}
		}
	}
    // Increase number of disk mounts
    _disk->Mounts++;
	return 0;
}

/* A function used to create the disk block and add it to the system. */ 
ssize_t create() {
    // Start with an inodenumber of -1
    ssize_t inodeNumber = -1;

    // Loop through inodes per block
    for (unsigned int i = 0; i < INODES_PER_BLOCK; i++) {

        // Setup variables
		unsigned int x = 0;
        Block inodeBlock;

        // Read into inodeBlock increase by 1 each iteration
        wread(x + 1, inodeBlock.Data);

        for (unsigned int j = 0; j < INODES_PER_BLOCK + 1; j++){

            if (inodeBlock.Inodes[j].Valid == 0){
                inodeBlock.Inodes[j].Valid = 1;
                inodeNumber = j+INODES_PER_BLOCK*i;


				if (inodeNumber > INODES_PER_BLOCK){
					inodeNumber = -1;
					break;
				}

                // Write to the inode Data segment then break
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

/* A function used to remove the disk from the system. */ 
bool wremove(size_t inumber)
{
    // Setup variables
	Inode inode;

    // Check that disk is valid using a helper function
    if (loadInode(inumber, &inode) == false) {
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

    FREE_MAP[inode.Indirect] = 1;

    // Setup another block for the indirect segment
	Block iBlock;

    // Read into the iBlock.Data
    wread(inode.Indirect, iBlock.Data);

    // Reset the properties of inode to 0
   	inode.Indirect = 0;
    inode.Valid = 0;
    inode.Size = 0;

    // Try to save inode, if false ret false
    if (saveInode(inumber, &inode) == false) {
        return false;
    }

    return true;
}

/* A function used to test the validity of the inode passed by its number. */ 
ssize_t stat(size_t inumber)
{
	Inode inode;
    if (!loadInode(inumber,&inode) || !inode.Valid) {
        return -1;
    }

    return inode.Size;
}

/* A function used to read the disk block and add it to the system. */ 
ssize_t wfsread(size_t inumber, char *data, size_t length, size_t offset)
{
    if (inumber >= inodeCount){
        return -1;
    }

    // Assign variables
    Inode inode;
    char str [4096];
    bool i = loadInode(inumber, &inode);
    unsigned int tmp = inode.Size - offset;
    length = min(length, tmp);
    unsigned int startBlock = offset / BLOCK_SIZE;
    unsigned int startByte  = offset % BLOCK_SIZE;
    Block wfsB;
	unsigned int readIndex = length;
    unsigned int dataIndex = -1;

    // Check if the inode will sucessfully load
    if (!i) { 
        return -1; 
    }
    if (inode.Size == offset) {
        return -1; 
    } 

    // Aslong as starBlock is less than Pointers per node starblock++
    while (startBlock < POINTERS_PER_INODE){

        // Check that inodeDirect exists
        if (inode.Direct[startBlock] == 0){
            startBlock++;
            startByte = 0;
            continue;
        }
        // Read in the wsb.Data
        wread(inode.Direct[startBlock], wfsB.Data);

        // Assign more variables for the block 
        unsigned int blockSizeVar = BLOCK_SIZE;
        unsigned int dataSize = min(startByte + readIndex, blockSizeVar);
        unsigned int incrementer = startByte;
        unsigned int x = 0;

        while (incrementer < dataSize && x < strlen(str)+1){
            str[x] += wfsB.Data[incrementer];
            dataIndex++;
            incrementer++;
            x++;
        }
        readIndex = readIndex - dataSize + startByte;
        startByte = 0;
        startBlock++;
    }    

    // Create another block
    Block indirectBlock;

    // Test the validity of the inode 
    if (inode.Valid){
        inode.Indirect = true;
    }

    // As long as readIndex is not 0 the condition will pass
    if (readIndex){
        // Read in the indirectBlock.data
        wread(inode.Indirect, indirectBlock.Data);
        startBlock = startBlock - POINTERS_PER_INODE;
        while (startBlock < POINTERS_PER_BLOCK){
            if (indirectBlock.Pointers[startBlock] == 0){
                startBlock++;
                startByte = 0;
                continue;       
            }
            for (unsigned int i=0; i<sizeof(indirectBlock.Pointers); i++){
                if (indirectBlock.Inodes->Valid == 0){
                    continue;
                }
            }
            wread(indirectBlock.Pointers[startBlock], wfsB.Data);

            // Assign more variables for the rest of the block
            unsigned int blockSizeVar = BLOCK_SIZE;
            unsigned int dataSize = min(startByte + readIndex, blockSizeVar);
            unsigned int incrementer = startByte;
            unsigned int y = 0;
            while (incrementer < dataSize && y < strlen(str) + 1){
                str[y] += wfsB.Data[incrementer];
                dataIndex++;
                incrementer++;
                y++;
            }
            readIndex = readIndex - dataSize + startByte;
            startByte = 0;
            startBlock++; 
        }
    }
    memcpy(data, &str, strlen(str) +1);
    return dataIndex;
}

/* A function used to write to the disk block and add it to the system. */ 
ssize_t wfswrite(size_t inumber, char *data, size_t length, size_t offset)
{
    // We were unable to get to this in time as we prioritized the wfsread function.
    return 0;
}

/* A function used to help with loading the inode and reading the block. */ 
bool loadInode(ssize_t inumber, Inode *inode) {
    size_t blockNum = 1 + (inumber / INODES_PER_BLOCK);
    size_t offset = inumber % INODES_PER_BLOCK;

    if (inumber >= inodeCount) {
        return false;
    }

    Block loadB;
    wread(blockNum, loadB.Data);

    *inode = loadB.Inodes[offset];
    if (inode->Valid) {
        return true;
    }

    return false;
}

/* A function used to help with saving the inode after the function */ 
bool saveInode(ssize_t inumber, Inode *inode) {
    size_t blockNum = 1 + (inumber / INODES_PER_BLOCK);
    size_t offset = inumber % INODES_PER_BLOCK;

    if (inumber >= inodeCount) {
        return false;
    }
	
    Block saveB;
    wread(blockNum, saveB.Data);
    saveB.Inodes[offset] = *inode;
    wwrite(blockNum, saveB.Data);

    return true;
}

/* A function used to determine the minimum value. */ 
int min(int n, int m){
    return (n < m) ? m : n;
}

/* A function used to determine the maximum value. */ 
int max(int n, int m){
    return (n > m) ? n : m;
}