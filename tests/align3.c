/* many odd sized allocations checked for alignment */
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "mem.h"

int main() {
    assert(Mem_Init(4096) == 0);
    void* ptr[9];
    ptr[0] = Mem_Alloc(1);	//Rounds to 8
    ptr[1] = (Mem_Alloc(5));	//Rounds to 16
    ptr[2] = (Mem_Alloc(14));	//Rounds to 24
    ptr[3] = (Mem_Alloc(8));	//Rounds to 16
    ptr[4] = (Mem_Alloc(1));	//Rounds to 8
    ptr[5] = (Mem_Alloc(4));	//Rounds to 8
    ptr[6] = (Mem_Alloc(9));	//Rounds to 16
    ptr[7] = (Mem_Alloc(33));	//Rounds to 40
    ptr[8] = (Mem_Alloc(55));	//Rounds to 64
   
    for (int i = 0; i < 9; i++) {
        assert(((int)ptr[i]) % 8 == 0);
    }
	Mem_Dump();
    exit(0);
}
