/* many odd sized allocations and interspersed frees */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   void * ptr[9];
   ptr[0] = Mem_Alloc(1);//dist 8
   ptr[1] = (Mem_Alloc(5));///dist 16
   ptr[2] = (Mem_Alloc(14));//dist 24
   ptr[3] = (Mem_Alloc(8));//dist 16
   assert(ptr[0] != NULL);
   assert(ptr[1] != NULL);
   assert(ptr[2] != NULL);
   assert(ptr[3] != NULL);
   //TODO comment out shit that DNW
   assert(Mem_Free(ptr[1]) == 0); //Free 16-No coalesce
   assert(Mem_Free(ptr[0]) == 0); //Free 8 - coalesce with free 16, get 24
   assert(Mem_Free(ptr[3]) == 0); //Free 16- coalesce with nothing (not adjacent).
   	//Total freesize should be 40
   ptr[4] = (Mem_Alloc(1));//dist 8
   ptr[5] = (Mem_Alloc(4));//dist 8
   
   assert(ptr[4] != NULL);
   assert(ptr[5] != NULL);
   
   assert(Mem_Free(ptr[5]) == 0);//Free 8. 
   
   ptr[6] = (Mem_Alloc(9)); //Dist 16
   ptr[7] = (Mem_Alloc(33));//dist 40
   assert(ptr[6] != NULL);//Free 16
   assert(ptr[7] != NULL);//free 40
   
   assert(Mem_Free(ptr[4]) == 0);//free 8 ball crack.

   ptr[8] = (Mem_Alloc(55)); //dist 64
   assert(ptr[8] != NULL);

   assert(Mem_Free(ptr[2]) == 0);
   assert(Mem_Free(ptr[7]) == 0);
   assert(Mem_Free(ptr[8]) == 0);
   assert(Mem_Free(ptr[6]) == 0);
	Mem_Dump();
   exit(0);
}
