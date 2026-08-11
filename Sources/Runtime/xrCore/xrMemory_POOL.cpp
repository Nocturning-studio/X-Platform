#include "stdafx.h"
#pragma hdrstop

#include "xrMemory_POOL.h"
#include "xrMemory_align.h"

void MEMPOOL::block_create()
{
    // Allocate
    R_ASSERT(0 == list);
    list = (u8*)xr_aligned_offset_malloc(s_sector, 16, s_offset);

#ifdef DEBUG_MEMORY_MANAGER
    _sector_start = list;   // remember the sector base address
#endif

    // Partition
    for (u32 it = 0; it < (s_count - 1); it++)
    {
        u8* E = list + it * s_element;
        *access(E) = E + s_element;
    }
    *access(list + (s_count - 1) * s_element) = NULL;
    block_count++;
}
