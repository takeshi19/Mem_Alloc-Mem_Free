/* write to a chunk from Mem_Alloc and check the value*/
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   int* ptr = (int*) Mem_Alloc(sizeof(int)); //Sizeof int is 4bytes.
   assert(ptr != NULL); //ptr --> 4_Bytes.
   *ptr = 42;   // check pointer is in a writeable page
   assert(*ptr == 42); //TODO what does this even mean? (42 == 42?) return true
   Mem_Dump();
   exit(0);
}
