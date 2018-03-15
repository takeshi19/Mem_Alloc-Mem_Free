/* a few allocations in multiples of 4 bytes followed by frees */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   void* ptr[4];

   ptr[0] = Mem_Alloc(4);	//8 bytes busy
   ptr[1] = Mem_Alloc(8);	//16 bytes also busy
  // assert(Mem_Free(ptr[0]) == 0); //Should free 8 bytes. 16 free still.
	//GOOD: we free 1 block successfully.
 	//Also good: freeing any size works, so nothing hardcoded...  
  // assert(Mem_Free(ptr[1]) == 0); //Should free successfully
	//FIXME why does freeing again cause buglife? 
   ptr[2] = Mem_Alloc(16);	  //SHould also free successfully.
//  ptr[3] = Mem_Alloc(4);
  assert(Mem_Free(ptr[2]) == 0);
  //assert(Mem_Free(ptr[3]) == 0);
  Mem_Dump();
   exit(0);
}
