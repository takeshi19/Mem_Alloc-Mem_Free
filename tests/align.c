/* first pointer returned is 8-byte aligned */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   int* ptr = (int*) Mem_Alloc(sizeof(int));
   assert(ptr != NULL);
   assert(((int)ptr) % 8 == 0); 
	printf("%08x\n", (unsigned int)(ptr));	
  
//Mem_Dump();
   exit(0);
}
