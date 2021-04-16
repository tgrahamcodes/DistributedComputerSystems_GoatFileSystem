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
	Block mountSb;
	wread(0,mountSb.Data);

	if (mountSb.Super.MagicNumber != MAGIC_NUMBER){
		return 1;
	}

	if (mountSb.Super.Blocks < 0){
		return 1;
	}

	if (mountSb.Super.Inodes != (mountSb.Super.InodeBlocks * INODES_PER_BLOCK)){
		return 1;
	}
	if (mountSb.Super.InodeBlocks != ceil(.1 * mountSb.Super.Blocks)) {
        return 1;
    } 

	blockCount = mountSb.Super.Blocks;
	inodeCount = mountSb.Super.Inodes;
	inodeBlocks = mountSb.Super.InodeBlocks;

	memset(FREE_MAP,0,BLOCK_SIZE);

	// printf("FREE_MAP = %u \n", (unsigned int)sizeof(FREE_MAP));
	for (unsigned int i=0; i < blockCount; i++){
		FREE_MAP[i] = 1;
	}

	FREE_MAP[0] = 0;

	for (unsigned int j=0; j < inodeBlocks; j++){
		FREE_MAP[j+1] = 0;
	}
	

	for (unsigned int x=0; x < inodeBlocks; x++){
		Block mountB;
		wread(x+1, mountB.Data);
		
		for (unsigned int n=0; n < INODES_PER_BLOCK; n++){
			if(!mountB.Inodes[n].Valid){
				continue;
			}
			unsigned int b = ceil(mountB.Inodes[n].Size/(double)BLOCK_SIZE);
			
			for (unsigned int e=0; e < POINTERS_PER_INODE && e < b; e++){
				FREE_MAP[mountB.Inodes[n].Direct[e]] = 0;
			}

			if (b > POINTERS_PER_INODE){
				Block indB;
				wread(mountB.Inodes[n].Indirect, indB.Data);
				FREE_MAP[mountB.Inodes[n].Indirect] = 0;
				for (unsigned int l = 0; l < b - POINTERS_PER_INODE; l++){
					FREE_MAP[indB.Pointers[l]] = 0;
				}
			}
		}
	}
	return 0;
}

void initializeInode(Inode *node) {
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
                initializeInode(&inodeBlock.Inodes[j]);
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
	Block iBlock;
    wread(inode.Indirect, iBlock.Data);

   	inode.Indirect = 0;
    inode.Valid = 0;
    inode.Size = 0;
    if (saveInode(inumber, &inode) == 0) {
        return false;
    }

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
    Inode inode;
    if (!loadInode(inumber, &inode) || offset > inode.Size) {
        return -1;
    }
	printf("debug 0: ");
	if (length < (inode.Size - offset)){
		inode.Size = length;
	}

    uint32_t start_block = offset / BLOCK_SIZE;

	printf("debug 1: ");
    Block wfsB;
	int ol = offset + length;
    if ((ol / BLOCK_SIZE) > POINTERS_PER_INODE) {
		if (inode.Indirect != 0) {
            return -1;
        }
        wread(inode.Indirect, wfsB.Data);
    }
	printf("debug 4: ");
    size_t read = 0;
    for (uint32_t block_num = start_block; read < length; block_num++) {
        size_t block_to_read;
        if (block_num < POINTERS_PER_INODE) {
            block_to_read = inode.Direct[block_num];
        } else {
            block_to_read = wfsB.Pointers[block_num-POINTERS_PER_INODE];
        }

        if (block_to_read == 0) {
            return -1;
        }

        Block b;
        wread(block_to_read,b.Data);
        size_t read_offset;
        size_t read_length;

        if (read == 0) {
            read_offset = offset % BLOCK_SIZE;
            read_length = BLOCK_SIZE - read_offset + length;
        } else {
            read_offset = 0;
            read_length = BLOCK_SIZE - length-read;
        }
        memcpy(data + read, b.Data + read_offset, read_length);
        read += read_length;
    }
    return read;
}

ssize_t wfswrite(size_t inumber, char *data, size_t length, size_t offset)
{
	Inode inode;
    if (!loadInode(inumber, &inode) || offset > inode.Size) {
        return -1;
    }

    size_t MAX_FILE_SIZE = BLOCK_SIZE * (POINTERS_PER_INODE*POINTERS_PER_BLOCK);

	if (length < (MAX_FILE_SIZE - offset)){
		inode.Size = length;
	}
    
    uint32_t start_block = offset / BLOCK_SIZE;
    Block indirect;
    bool read_indirect = false;

    bool modified_inode = false;
    bool modified_indirect = false;

    size_t written = 0;
    for (uint32_t block_num = start_block; written < length && block_num < POINTERS_PER_INODE + POINTERS_PER_BLOCK; block_num++) {
        size_t block_to_write;
        if (block_num < POINTERS_PER_INODE) {
            if (inode.Direct[block_num] == 0) {
                ssize_t allocated_block = freeBlock();
                if (allocated_block == -1) {
                    break;
                }
                inode.Direct[block_num] = allocated_block;
                modified_inode = true;
            }
            block_to_write = inode.Direct[block_num];
        } else {
            if (inode.Indirect == 0) {
                ssize_t allocated_block = freeBlock();
                if (allocated_block == -1) {
                    return written;
                }
                inode.Indirect = allocated_block;
                modified_indirect = true;
            }
            if (!read_indirect) {
                wread(inode.Indirect,indirect.Data);
                read_indirect = true;
            }

            if (indirect.Pointers[block_num - POINTERS_PER_INODE] == 0) {
                ssize_t allocated_block = freeBlock();
                if (allocated_block == -1) {
                    break;
                }
                indirect.Pointers[block_num - POINTERS_PER_INODE] = allocated_block;
                modified_indirect = true;
            }
            block_to_write = indirect.Pointers[block_num-POINTERS_PER_INODE];
        }
        size_t write_offset;
        size_t write_length;
        if (written == 0) {
            write_offset = offset % BLOCK_SIZE;
			if (write_length < (BLOCK_SIZE - write_offset)){
				write_offset = write_length;
			}
        }else{
            write_offset = 0;
			if (write_length > BLOCK_SIZE){
				write_length = BLOCK_SIZE;
			}
			else if (write_length > (length - written)){
				write_length = length - written;
			}
        }
        char write_buffer[BLOCK_SIZE];
        if (write_length < BLOCK_SIZE) {
            wread(block_to_write,(char*)write_buffer);
        }
        memcpy(write_buffer + write_offset, data + written, write_length);
        wwrite(block_to_write,(char*)write_buffer);
        written += write_length;
    }

    uint32_t new_size = (size_t)inode.Size;
	if (new_size < (written + offset)){
		new_size = (written + offset);
	}else if ((new_size < inode.Size)){
		new_size = inode.Size;
	}

    if (new_size != inode.Size) {
        inode.Size = new_size;
        modified_inode = true;
    }

    if (modified_inode) {
        saveInode(inumber,&inode);
    }
     
    if (modified_indirect) {
        wwrite(inode.Indirect,indirect.Data);
    }

    return written;
}

bool loadInode(ssize_t inumber, Inode *inode) {
    size_t blockNum = 1 + (inumber / INODES_PER_BLOCK);
    size_t offset = inumber % INODES_PER_BLOCK;

    if (inumber >= inodeCount) {
        return false;
    }

    Block loadB;
    wread(blockNum, loadB.Data);

    *inode = loadB.Inodes[offset];

    return true;
}

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

ssize_t freeBlock(){
	int block = -1;
    for (unsigned int i = 0; i < blockCount; i++) {
        if (FREE_MAP[i]) {
            FREE_MAP[i] = 0;
            block = i;
            break;
        }
    }

    if (block != -1) {
        char data[BLOCK_SIZE];
        memset(data,0,BLOCK_SIZE);
        wwrite(block,(char*)data);
    }

    return block;
}