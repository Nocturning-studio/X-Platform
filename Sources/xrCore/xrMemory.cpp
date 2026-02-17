#include "stdafx.h"
#pragma hdrstop

#include "xrsharedmem.h"
#include "xrMemory_pure.h"

#include <malloc.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

xrMemory Memory;
BOOL mem_initialized = FALSE;
bool shared_str_initialized = false;

#ifdef DEBUG_MEMORY_MANAGER
XRCORE_API void dump_phase()
{
	if (!Memory.debug_mode)
		return;

	static int phase_counter = 0;
	char temp[256];
	sprintf_s(temp, sizeof(temp), "x:\\$phase$%d.dump", ++phase_counter);
	Memory.mem_statistic(temp);
}
#endif // DEBUG_MEMORY_MANAGER

xrMemory::xrMemory()
#ifdef DEBUG_MEMORY_MANAGER
#ifdef PROFILE_CRITICAL_SECTIONS
	: debug_cs(MUTEX_PROFILE_ID(xrMemory))
#endif
#endif
{
#ifdef DEBUG_MEMORY_MANAGER
	debug_mode = FALSE;
#endif
	// ”казатели на PSO больше не нужны
}

#ifdef DEBUG_MEMORY_MANAGER
BOOL g_bMEMO = FALSE;
#endif

void xrMemory::_initialize(BOOL bDebug)
{
#ifdef DEBUG_MEMORY_MANAGER
	debug_mode = bDebug;
	debug_info_update = 0;
#endif

	stat_calls = 0;
	stat_counter = 0;

	const char* cmdLine = Core.Params;
	if (!strstr(cmdLine, "-pure_alloc"))
	{
		u32 element = mem_pools_ebase;
		u32 sector = mem_pools_ebase * 1024;
		for (u32 pid = 0; pid < mem_pools_count; ++pid)
		{
			mem_pools[pid]._initialize(element, sector, 0x1);
			element += mem_pools_ebase;
		}
	}

#ifdef DEBUG_MEMORY_MANAGER
	if (strstr(cmdLine, "-memo") == nullptr)
		mem_initialized = TRUE;
	else
		g_bMEMO = TRUE;
#else
	mem_initialized = TRUE;
#endif

	g_pStringContainer = xr_new<str_container>();
	shared_str_initialized = true;
	g_pSharedMemoryContainer = xr_new<smem_container>();
}

#ifdef DEBUG_MEMORY_MANAGER
extern void dbg_dump_leaks();
extern void dbg_dump_str_leaks();
#endif

void xrMemory::_destroy()
{
#ifdef DEBUG_MEMORY_MANAGER
	mem_alloc_gather_stats(false);
	mem_alloc_show_stats();
	mem_alloc_clear_stats();

	if (debug_mode)
		dbg_dump_str_leaks();
#endif

	xr_delete(g_pSharedMemoryContainer);
	xr_delete(g_pStringContainer);

#ifdef DEBUG_MEMORY_MANAGER
	if (debug_mode)
		dbg_dump_leaks();
#endif

	mem_initialized = FALSE;
#ifdef DEBUG_MEMORY_MANAGER
	debug_mode = FALSE;
#endif
}

void xrMemory::mem_compact()
{
	RegFlushKey(HKEY_CLASSES_ROOT);
	RegFlushKey(HKEY_CURRENT_USER);
	_heapmin();
	HeapCompact(GetProcessHeap(), 0);

	if (g_pStringContainer)
		g_pStringContainer->clean();
	if (g_pSharedMemoryContainer)
		g_pSharedMemoryContainer->clean();

	if (strstr(Core.Params, "-swap_on_compact"))
		SetProcessWorkingSetSize(GetCurrentProcess(), size_t(-1), size_t(-1));
}

#ifdef DEBUG_MEMORY_MANAGER
ICF u8* acc_header(void* P)
{
	return (u8*)P - 1;
}
ICF u32 get_header(void* P)
{
	return (u32)*acc_header(P);
}

void xrMemory::mem_statistic(LPCSTR fn)
{
	if (!debug_mode)
		return;
	mem_compact();

	debug_cs.Enter();
	debug_mode = FALSE;

	FILE* Fa = fopen(fn, "w");
	fprintf(Fa, "$BEGIN CHUNK #0\n");
	fprintf(Fa, "POOL: %d %dKb\n", mem_pools_count, mem_pools_ebase);

	fprintf(Fa, "$BEGIN CHUNK #1\n");
	for (u32 k = 0; k < mem_pools_count; ++k)
		fprintf(Fa, "%2d: %d %db\n", k, mem_pools[k].get_block_count(), (k + 1) * 16);

	fprintf(Fa, "$BEGIN CHUNK #2\n");
	for (size_t it = 0; it < debug_info.size(); ++it)
	{
		if (debug_info[it]._p == nullptr)
			continue;

		u32 p_current = get_header(debug_info[it]._p);
		int pool_id = (mem_generic == p_current) ? -1 : p_current;

		fprintf(Fa, "0x%p[%2d]: %8zu %s\n", debug_info[it]._p, pool_id, debug_info[it]._size, debug_info[it]._name);
	}

	{
		for (u32 k = 0; k < mem_pools_count; ++k)
		{
			MEMPOOL& pool = mem_pools[k];
			u8* list = pool.list;
			while (list)
			{
				pool.cs.Enter();
				u8* next = *reinterpret_cast<u8**>(list);
				fprintf(Fa, "0x%p[%2d]: %8u mempool\n", list, k, pool.s_element);
				list = next;
				pool.cs.Leave();
			}
		}
	}

	fclose(Fa);

	debug_mode = TRUE;
	debug_cs.Leave();
}
#endif // DEBUG_MEMORY_MANAGER

char* xr_strdup(const char* string)
{
	VERIFY(string);
	size_t len = std::strlen(string) + 1;
	char* memory = static_cast<char*>(Memory.mem_alloc(len
#ifdef DEBUG_MEMORY_NAME
													   ,
													   "strdup"
#endif
													   ));
	std::memcpy(memory, string, len);
	return memory;
}

XRCORE_API BOOL is_stack_ptr(void* _ptr)
{
	int local_value = 0;
	void* ptr_local = &local_value;
	ptrdiff_t difference =
		std::abs(static_cast<ptrdiff_t>(reinterpret_cast<char*>(ptr_local) - reinterpret_cast<char*>(_ptr)));
	return (difference < (512 * 1024));
}
