////////////////////////////////////////////////////////////////////////////////
// Created: 02.11.2026 21:54:28
// Author: NS_Deathman
// File: SceneGraphArenaAllocator.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class SceneGraphArena
{
  public:
	  void Init(size_t bytes)
	  {
		  if (m_base)
			  Destroy();

		  m_base = static_cast<u8*>(xr_malloc(bytes));
		  m_capacity = bytes;
		  m_offset = 0;
		  m_peak = 0;
	  }

	void Destroy()
	{
		for(void* p : m_fallbacks)
			xr_free(p);
		m_fallbacks.clear();
		xr_free(m_base);
		m_base = nullptr;
		m_capacity = m_offset = m_peak = 0;
	}

	void Reset()
	{
		for(void* p : m_fallbacks)
			xr_free(p);
		m_fallbacks.clear();
		m_offset = 0;
	}

	[[nodiscard]] void* Alloc(size_t bytes, size_t align = alignof(std::max_align_t))
	{
		const size_t aligned = (m_offset + align - 1) & ~(align - 1);
		if(aligned + bytes <= m_capacity)
		{
			void* p = m_base + aligned;
			m_offset = aligned + bytes;
			if(m_offset > m_peak)
				m_peak = m_offset;
			return p;
		}
		void* p = xr_malloc(static_cast<u32>(bytes));
		m_fallbacks.push_back(p);
		return p;
	}

	size_t Peak() const { return m_peak; }
	size_t Used() const { return m_offset; }
	size_t Capacity() const { return m_capacity; }

  private:
	u8* m_base = nullptr;
	size_t m_capacity = 0;
	size_t m_offset = 0;
	size_t m_peak = 0;
	xr_vector<void*> m_fallbacks;
};

template <typename T>
class ArenaAlloc
{
public:
	using value_type = T;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;

	using propagate_on_container_swap = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_copy_assignment = std::true_type;
	using is_always_equal = std::false_type;


	template <typename U>
	struct rebind { using other = ArenaAlloc<U>; };

	ArenaAlloc() noexcept : m_arena(nullptr) {}
	explicit ArenaAlloc(SceneGraphArena* a) noexcept : m_arena(a) {}
	template <typename U> ArenaAlloc(const ArenaAlloc<U>& o) noexcept : m_arena(o.m_arena) {}

	T* allocate(size_t n)
	{
		return static_cast<T*>(m_arena ? m_arena->Alloc(n * sizeof(T), alignof(T)) : xr_malloc(static_cast<u32>(n * sizeof(T))));
	}

	void deallocate(T* p, size_t) noexcept
	{
		if (!m_arena && p)
			xr_free(p);
	}

	template <typename U> bool operator==(const ArenaAlloc<U>& o) const noexcept { return m_arena == o.m_arena; }
	template <typename U> bool operator!=(const ArenaAlloc<U>& o) const noexcept { return m_arena != o.m_arena; }

	SceneGraphArena* arena() const noexcept { return m_arena; }

private:
	SceneGraphArena* m_arena;
	template <typename U> friend class ArenaAlloc;
};
////////////////////////////////////////////////////////////////////////////////
