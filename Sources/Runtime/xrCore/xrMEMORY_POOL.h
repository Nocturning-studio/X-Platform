#ifndef xrMemory_POOLh
#define xrMemory_POOLh
#pragma once

class xrMemory;

class MEMPOOL
{
#ifdef DEBUG_MEMORY_MANAGER
    friend class xrMemory;
#endif

private:
    xrCriticalSection cs;
    u32 s_sector;        // large-memory sector size
    u32 s_element;       // element size, e.g. 32
    u32 s_count;         // element count = s_sector / s_element
    u32 s_offset;        // header size
    u32 block_count;     // number of allocated sectors
    u8* list;           // free list head

#ifdef DEBUG_MEMORY_MANAGER
    // ---- debug fields ----
    u32  _guard_before;  // canary before list
    u8* _sector_start;  // start of last allocated sector (for boundary checks)
    u32  _guard_after;   // canary after list

    static const u32 GUARD_VALUE = 0xDEADBEEF;

    void check_guards() const
    {
        if (_guard_before != GUARD_VALUE || _guard_after != GUARD_VALUE)
        {
            // MEMPOOL object itself corrupted
            __debugbreak();
        }
    }
#endif

private:
    ICF void** access(void* P)
    {
        return (void**)((void*)(P));
    }
    void block_create();

public:
    void _initialize(u32 _element, u32 _sector, u32 _header)
    {
        R_ASSERT(_element < _sector / 2);
        s_sector = _sector;
        s_element = _element;
        s_count = s_sector / s_element;
        s_offset = _header;
        list = NULL;
        block_count = 0;

#ifdef DEBUG_MEMORY_MANAGER
        _sector_start = nullptr;
        _guard_before = GUARD_VALUE;
        _guard_after = GUARD_VALUE;
#endif
    }

#ifdef PROFILE_CRITICAL_SECTIONS
    ICF MEMPOOL() : cs(MUTEX_PROFILE_ID(memory_pool))
    {
# ifdef DEBUG_MEMORY_MANAGER
        _sector_start = nullptr;
        _guard_before = GUARD_VALUE;
        _guard_after = GUARD_VALUE;
# endif
    }
#endif

    ICF u32 get_block_count() const
    {
#ifdef DEBUG_MEMORY_MANAGER
        check_guards();
#endif
        return block_count;
    }

    ICF u32 get_element() const
    {
#ifdef DEBUG_MEMORY_MANAGER
        check_guards();
#endif
        return s_element;
    }

    ICF void* create()
    {
#ifdef DEBUG_MEMORY_MANAGER
        check_guards();
#endif
        cs.Enter();
        if (0 == list)
            block_create();

        // list must be valid and inside sector (if sector known)
        if (IsBadReadPtr(list, sizeof(void*)) || list == (void*)1)
        {
            __debugbreak();
        }

        void* E = list;
        list = (u8*)*access(list);   // advance to next free element

        cs.Leave();
        return E;
    }

    ICF void destroy(void*& P)
    {
#ifdef DEBUG_MEMORY_MANAGER
        check_guards();
#endif
        cs.Enter();

        if (IsBadReadPtr(P, sizeof(void*)))
        {
            __debugbreak();
        }

        *access(P) = list;
        list = (u8*)P;
        cs.Leave();
    }
};

#endif // xrMemory_POOLh
