CS4513: Project 1 Goat File System
==================================

Note, this document includes a number of design questions that can help your implementation. We highly recommend that you answer each design question **before** attempting the corresponding implementation.
These questions will help you design and plan your implementation and guide you towards the resources you need.
Finally, if you are unsure how to start the project, we recommend you visit office hours for some guidance on these questions before attempting to implement this project.


Team members
-----------------

1. Thomas Graham
2. Lorenzo Lopez (lplopez@wpi.edu)

Design Questions
------------------

1. When implementing the `debug()` function, you will need to load the file system via the emulated disk and retrieve the information for superblock and inodes.
1.1 How will you read the superblock?
We read the superblock by calling wread(0, block.Data) and by using super we can get the magic number
1.2 How will you traverse all the inodes?
We traverse the inodes by reading at block 1 and iterating through them.
1.3 How will you determine all the information related to an inode?
We determine this info related to an inode by reading the fields the struct Inode has.
1.4 How will you determine all the blocks related to an inode?
We can determine that all blocks have been read when Inode.Direct has nothing to point to.
---

2. When implementing the `format()` function, you will need to write the superblock and clear the remaining blocks in the file system.

2.1 What should happen if the the file system is already mounted?
This system call should not succeed as the disk cannot be mounted.
2.2 What information must be written into the superblock?
The magic number, the number of blocks, the number of inode blocks and the number of inodes must be written.
2.3 How would you clear all the remaining blocks?
Set the Valid field of the inodes parsed to zero.

---

3. When implementing the `mount()` function, you will need to prepare a filesystem for use by reading the superblock and allocating the free block bitmap.

3.1 What should happen if the file system is already mounted?
This system call should not succeed as the disk cannot be mounted.
3.2 What sanity checks must you perform before building up the free block bitmaps?
The magic number must be correct, ie. the blocks on the disk must be the same as described by the superblock. The same can be said for corresponding inodes.
3.3 How will you determine which blocks are free?
We will know this by parsing the blocks and if any blocks are pointing to valid inodes.


---

4. To implement `create()`, you will need to locate a free inode and save a new inode into the inode table.

4.1 How will you locate a free inode?
We can traverse through the inodes until we find a valid unsigned int of 0.
4.2 What information would you see in a new inode?
New nodes would be default, they would have a length of 0, 0 for valid, the direct pointers would be pointing to null. The indirect pointer would point to an indirect block with the indirect block pointers will be pointing to null.
4.3 How will you record this new inode?
The superblock will reflect this change as the number of inodes has increased. 



---

5. To implement `remove()`, you will need to locate the inode and then free its associated blocks.

5.1 How will you determine if the specified inode is valid?
We need to check the inode.Valid variable, by doing this we can see if its either a 0 or a 1.
5.2 How will you free the direct blocks?
In order to free the blocks we need to get all the information for pointers in the inode. After this we can then update the free block bitmap for each block that the inode pointers are pointing to. Set this information to null, existent data will be over-rided. 
5.3 How will you free the indirect blocks?
Similar to how we did the freeing of direct blocks, we will do the same for indirect block. We will update the free block bitmap for each block that inode pointers are pointing to. Set this information to null, existent data will be over-rided.
5.4 How will you update the inode table?
By decreasing the number of inodes in the superblock. Similar to how we created a new inode, each removed inode will have a valid variable changed to 0 where it was previously 1. 



---

6. To implement `stat()`, you will need to locate the inode and return its size.

6.1 How will you determine if the specified inode is valid?
To determine if the specified inode is valid we need to look into the superblock, specifically the inode table and when we reach the desired inode check the valid variable. 
6.2 How will you determine the inode's size?
Simply follow the answer above but instead of checking the balid variable, we will check the size variable of the inode. 



---

7. To implement `read()`, you will need to locate the inode and copy data from appropriate blocks to the user-specified data buffer.

7.1  How will you determine if the specified inode is valid?
To determine if the specified inode is valid we need to look into the superblock, specifically the inode table and when we reach the desired inode check the valid variable.
7.2  How will you determine which block to read from?
To determine which block we are reading from we will divide the offset by the block size. From this point, we will start reading from a specified block. 
7.3  How will you handle the offset?
To handle the offset we need to divide it to by the block size. 
7.4  How will you copy from a block to the data buffer?
Take the specified information within the block and override it to the buffer so that the buffer now contains this information. 




---

8. To implement `write()`, you will need to locate the inode and copy data the user-specified data buffer to data blocks in the file system.

8.1  How will you determine if the specified inode is valid?
To determine if the specified inode is valid we need to look into the superblock, specifically the inode table and when we reach the desired inode check the valid variable.
8.2  How will you determine which block to write to?
To determine which block we will be writing to we will check the offset. 
8.3  How will you handle the offset?
To handle the offset we need to divide it to by the block size.
8.4  How will you know if you need a new block?
We will know if we need to create a new block if we have exceeded the number of blocks that have been allocated previously.
8.5  How will you manage allocating a new block if you need another one?
To manage the addition of a new block if we need another one we call look at the existing block list, look at the block we have added and assign a corresponding pointer to it from the inode.
8.6  How will you copy from a block to the data buffer?
Take the specified information within the block and override it to the buffer so that the buffer now contains this information. 
8.7  How will you update the inode?
To update an inode we will change the size of the inode and also the direct, indirect pointers.



---


Errata
------

Describe any known errors, bugs, or deviations from the requirements.

For create()
    We needed to pass the flag -fno-stack-protector, because we were getting "stack smashing errors", but after this was
        passed our create function works fine

---

(Optional) Additional Test Cases
--------------------------------

Describe any new test cases that you developed.

