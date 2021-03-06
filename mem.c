#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/*
 * This structure serves as the header for each allocated and free block
 * It also serves as the footer for each free block
 * The blocks are ordered in the increasing order of addresses 
 */
typedef struct blk_hdr {                         
        int size_status;
  
    /*
    * Size of the block is always a multiple of 8
    * => last two bits are always zero - can be used to store other information
    *
    * LSB -> Least Significant Bit (Last Bit)
    * SLB -> Second Last Bit 
    * LSB = 0 => free block
    * LSB = 1 => allocated/busy block
    * SLB = 0 => previous block is free
    * SLB = 1 => previous block is allocated/busy
    * 
    * When used as the footer the last two bits should be zero
    */

    /*
    * Examples:
    * 
    * For a busy block with a payload of 20 bytes (i.e. 20 bytes data + an additional 4 bytes for header)
    * Header:
    * If the previous block is allocated, size_status should be set to 27 (add 3)
    * If the previous block is free, size_status should be set to 25	  (add 1)
    * 
    * For a free block of size 24 bytes (including 4 bytes for header + 4 bytes for footer)
    * Header: (payload = 16 bytes) 
    * If the previous block is allocated, size_status should be set to 26
    * If the previous block is free, size_status should be set to 24
    * Footer:
    * size_status should be 24
    * 
    */
} blk_hdr;

/* Global variable - This will always point to the first block
 * i.e. the block with the lowest address */
blk_hdr *first_blk = NULL;

/*
 * Note: 
 *  The end of the available memory can be determined using end_mark
 *  The size_status of end_mark has a value of 1
 *
 */

/* 
 * Function for allocating 'size' bytes
 * Returns address of allocated block on success 
 * Returns NULL on failure 
 * Here is what this function should accomplish 
 * - Check for sanity of size - Return NULL when appropriate 
 * - Round up size to a multiple of 8 
 * - Traverse the list of blocks and allocate the best free block which can accommodate the requested size 
 * - Also, when allocating a block - split it into two blocks
 * Tips: Be careful with pointer arithmetic 
 */
void* Mem_Alloc(int size) {                      
	if (size <= 0) //Request of invalid amount of memory, return null.
		return NULL;  
	size += 4;     //Add 4 bytes for header to requested size.

	//**Padding**
	if (size % 8 != 0) {  
  		int difference; 
		if (size < 8) { //size is less than 8. 
			difference = 8 - size;
			size += difference;
		} 
		else { 		//size is > 8.
			difference = size / 8;
			int multiple = 8  * (difference + 1);
			size += (multiple - size); 
		}
	}
	
	//**Traversing heap to find best-fitting block to allocate for requested size**
	blk_hdr* memptr = first_blk;		 //memptr points to start of payload of 1st block.
	blk_hdr* best = NULL; 
	while (memptr->size_status != 1) { 
		int curr = memptr->size_status;  //Reinit curr to the next block's size.
		int currsize = curr - (curr & 3);//The size of every traversed block.
		if ((curr & 1) == 0 && currsize >= size) {  //If the block is free then we can choose optimal fit.
			if (currsize == size) {  //If we have an exact fit, break loop and allocate best block.
				best = memptr; 	 //best points to the potential best-fitting block.
				break;
			}
			if ( best == NULL || currsize <= best->size_status - (best->size_status & 3)) 
				best = memptr;		//Found new best-fit block.	
		}
	
		memptr = (blk_hdr*)((char*)(memptr) + currsize); //Go to next block.
	}
	//**If a big enough block was never found in heap, return NULL.**
	if (best == NULL) 
		return NULL;
	
 	//**Actual size of found available block.**
	int bestsize = best->size_status - (best->size_status & 3); 
	
	//**Allocating the block of best fit recently found.**
	blk_hdr *pload = (blk_hdr*)((char*)(best) + 4); //Pointer to start of payload of alloc'd block.
	if ((bestsize - size) >= 8) {	     //Split if there will be enough free space for a 2nd block.
		best->size_status = size + (best->size_status & 3) + 1;
		
		blk_hdr *freeHdr = (blk_hdr*)((char*)(best) + size);  //Move pointer to free block. 
		freeHdr->size_status = (bestsize-size) + 2;     //Update free block header.
			
		blk_hdr* freeFtr = (blk_hdr*)((char*)(best) + (bestsize-4));		
		freeFtr->size_status = bestsize - size;		//Update free block footer.
	}
	else { //If we cannot split, update header accordingly.
		best->size_status += 1;
		blk_hdr * nextblk = (blk_hdr*)((char*)(best) + bestsize);
		nextblk->size_status += 2; //Update header of next block.
	}	
	
	return (void *)pload;
}

/* 
 * Function for freeing up a previously allocated block 
 * Argument - ptr: Address of the block to be freed up 
 * Returns 0 on success 
 * Returns -1 on failure 
 * Here is what this function should accomplish 
 * - Return -1 if ptr is NULL
 * - Return -1 if ptr is not 8 byte aligned or if the block is already freed
 * - Mark the block as free 
 * - Coalesce if one or both of the immediate neighbours are free 
 */
int Mem_Free(void *ptr) {                        
	//**If either ptr is null or ptr isn't multiple 8, return -1.**
	if (!ptr || ((int)ptr) % 8 != 0) 
		return -1;
	//**Casting ptr to blk_hdr to access its header.**
	blk_hdr *freeme = (blk_hdr *)ptr;
	//**If ptr is before heap/out of bounds, return -1. 
	if (freeme < first_blk)  { //Before heap/out of bounds, return -1.
		printf("Out of bounds leftwise in memfree\n");
		return -1;
	}

	freeme = (blk_hdr*)((char*)(freeme) - 4); //Going to header.
	
	//**If ptr is already freed, return -1.**
	if ((freeme->size_status & 1) == 0) 
		return -1;
	
	blk_hdr *newfoot = freeme;	  
	freeme->size_status -= 1;  	  //Declaring the block as free.
	
	//**Making a footer for newly freed block.**
	int freePayload = freeme->size_status - (freeme->size_status & 3);
	newfoot = (blk_hdr*)((char*)(newfoot) + freePayload - 4);
	newfoot->size_status = freePayload;	

	//**Coalescing free blocks in heap.**
	switch (freeme->size_status & 3) {
		case 0: //Previous block is free, coalesce with freeme.
			freeme = (blk_hdr*)((char*)(freeme) + 4); 
			
			//**Move to footer of previous block (free).**
			blk_hdr *prevblk = (blk_hdr*)((char*)(freeme) - 8); 
			blk_hdr *footer = prevblk; 
			prevblk = (blk_hdr*)((char*)(prevblk) - (footer->size_status)+4);
			
			//**Updating header after coalescing.**
			footer = (blk_hdr*)((char*)(footer) + 4);
			prevblk->size_status += freePayload;
			
			//**Making/updating footer of newly coalesced block.**
			footer = (blk_hdr*)((char*)(footer) + freePayload-4);
			footer->size_status = prevblk->size_status - (prevblk->size_status&3);
			
			//**Going to header of next block.**
			blk_hdr *nextblk = freeme; 				    
			nextblk = (blk_hdr*)((char*)(nextblk) + freePayload-4);

			//**Update nextblk header, check if it's free.**
			nextblk->size_status -= 2; 
			if ((nextblk->size_status & 1) == 0) { 
				//**Go to footer of next block and update it.**
				nextblk = (blk_hdr*)((char*)(nextblk) + (nextblk->size_status)-4);
				nextblk->size_status += footer->size_status;
				//**Updating header of coalesced blk.**
				prevblk->size_status = nextblk->size_status + (prevblk->size_status&3); 
			}
			break;
		case 2: //Previous block is alloc'd, check if next block free.
			freeme = (blk_hdr*)((char*)(freeme) + 4); 
			//**Go to header of next block.**
			blk_hdr *newnext = (blk_hdr*)((char*)(freeme) + freePayload - 4);
			
			//**Making footer for current-free block.**
			blk_hdr *currhead = (blk_hdr*)((char*)(freeme) + freePayload - 8);
			currhead->size_status = freePayload;

			newnext->size_status -= 2; //The previous blk is free.
			//**Check if next is free.**
			if ((newnext->size_status & 1) == 0) { 
				//**Accessing header of free next block.**
				newnext = (blk_hdr*)((char*)(newnext) + (newnext->size_status)-4);
				newnext->size_status += freePayload; 
			
				//**Updating bigger size of freeme header.**
				freeme = (blk_hdr*)((char*)(freeme) - 4);
				freeme->size_status = newnext->size_status + (freeme->size_status&3); 
			}
			break;
	}
	
	//**If coalescing works &/or we freed pointer by request, return 0.**
	return 0;
}


/*
 * Function used to initialize the memory allocator
 * Not intended to be called more than once by a program
 * Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated
 * Returns 0 on success and -1 on failure 
 */
int Mem_Init(int sizeOfRegion) {                         
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    blk_hdr* end_mark;
    static int allocated_once = 0;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: Mem_Init has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, 
                    fd, 0);
    if (MAP_FAILED == space_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignement and end mark
    alloc_size -= 8;

    // To begin with there is only one big free block
    // initialize heap so that first block meets 
    // double word alignement requirement
    first_blk = (blk_hdr*) space_ptr + 1;
    end_mark = (blk_hdr*)((void*)first_blk + alloc_size); //end_mark is the ptr to the end of free list/heap.
  
    // Setting up the header
    first_blk->size_status = alloc_size;

    // Marking the previous block as busy
    first_blk->size_status += 2;

    // Setting up the end mark and marking it as busy
    end_mark->size_status = 1;

    // Setting up the footer
    blk_hdr *footer = (blk_hdr*) ((char*)first_blk + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
}

/* 
 * Function to be used for debugging 
 * Prints out a list of all the blocks along with the following information i
 * for each block 
 * No.      : serial number of the block 
 * Status   : free/busy 
 * Prev     : status of previous block free/busy
 * t_Begin  : address of the first byte in the block (this is where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block (as stored in the block header) (including the header/footer)
 */ 
void Mem_Dump() {                        
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end = NULL;
    int t_size;

    blk_hdr *current = first_blk;
    counter = 1;

    int busy_size = 0;
    int free_size = 0;
    int is_busy = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => busy block
            strcpy(status, "Busy");
            is_busy = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_busy = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "Busy");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_busy) 
            busy_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blk_hdr*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total busy size = %d\n", busy_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", busy_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;
}
