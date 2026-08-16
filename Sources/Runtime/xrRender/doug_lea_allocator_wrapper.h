#pragma once

#ifndef USE_MEMORY_MONITOR
#define USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#endif // USE_MEMORY_MONITOR

#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#include "doug_lea_memory_allocator.h"

template <class T> class doug_lea_alloc
{
public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;

    template <class _Other> struct rebind
    {
        using other = doug_lea_alloc<_Other>;
    };

    pointer address(reference _Val) const { return (&_Val); }
    const_pointer address(const_reference _Val) const { return (&_Val); }

    doug_lea_alloc() {}
    doug_lea_alloc(const doug_lea_alloc<T>&) {}

    template <class _Other> doug_lea_alloc(const doug_lea_alloc<_Other>&) {}

    template <class _Other> doug_lea_alloc<T>& operator=(const doug_lea_alloc<_Other>&)
    {
        return (*this);
    }

    pointer allocate(size_type n, const void* p = 0) const
    {
        return (T*)dlmalloc(sizeof(T) * (u32)n);
    }

    char* __charalloc(size_type n)
    {
        return (char*)allocate(n);
    }

    void deallocate(pointer p, size_type n) const
    {
        dlfree(p);
    }

    void deallocate(void* p, size_type n) const
    {
        dlfree(p);
    }

    void construct(pointer p, const T& _Val)
    {
        ::new ((void*)p) value_type(_Val);
    }

    void destroy(pointer p)
    {
        p->~T();
    }

    size_type max_size() const
    {
        size_type _Count = (size_type)(-1) / sizeof(T);
        return (0 < _Count ? _Count : 1);
    }
};

template <class _Ty, class _Other> inline bool operator==(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)
{
    return true;
}

template <class _Ty, class _Other> inline bool operator!=(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)
{
    return false;
}

struct doug_lea_allocator
{
    template <typename T> struct helper
    {
        using result = doug_lea_alloc<T>;
    };

    static void* alloc(const u32& n)
    {
        return dlmalloc((u32)n);
    }

    template <typename T> static void dealloc(T*& p)
    {
        dlfree(p);
        p = 0;
    }
};

// Aliases for usage in SceneGraph
#define render_alloc doug_lea_alloc
using render_allocator = doug_lea_allocator;

#else // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER

// Fallback to standard xr_allocator
#define render_alloc xalloc
using render_allocator = xr_allocator;

#endif // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
