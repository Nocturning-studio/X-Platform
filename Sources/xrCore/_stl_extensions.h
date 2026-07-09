#pragma once

#include <cstddef> // size_t, ptrdiff_t
#include <new>	   // placement new

// Аллокатор xalloc, использующий xr_alloc/xr_free
template <class T> class xalloc
{
  public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template <class U> struct rebind
	{
		typedef xalloc<U> other;
	};

	pointer address(reference val) const noexcept
	{
		return &val;
	}

	const_pointer address(const_reference val) const noexcept
	{
		return &val;
	}

	xalloc() noexcept = default;

	xalloc(const xalloc&) noexcept = default;

	template <class U> xalloc(const xalloc<U>&) noexcept
	{
	}

	template <class U> xalloc& operator=(const xalloc<U>&) noexcept
	{
		return *this;
	}

	pointer allocate(size_type n, const void* /*ptr*/ = nullptr) const
	{
		return static_cast<pointer>(xr_alloc<T>(static_cast<u32>(n)));
	}

	char* _charalloc(size_type n) const
	{
		return reinterpret_cast<char*>(allocate(n));
	}

	void deallocate(pointer ptr, size_type /*n*/) const noexcept
	{
		xr_free(ptr);
	}

	void deallocate(void* ptr, size_type /*n*/) const noexcept
	{
		xr_free(ptr);
	}

	void construct(pointer ptr, const T& val)
	{
		::new (static_cast<void*>(ptr)) T(val);
	}

	void destroy(pointer ptr) noexcept
	{
		ptr->~T();
	}

	size_type max_size() const noexcept
	{
		size_type count = static_cast<size_type>(-1) / sizeof(T);
		return (count > 0) ? count : 1;
	}
};

// Операторы сравнения (всегда true/false для любых экземпляров)
template <class T, class U> inline bool operator==(const xalloc<T>&, const xalloc<U>&) noexcept
{
	return true;
}

template <class T, class U> inline bool operator!=(const xalloc<T>&, const xalloc<U>&) noexcept
{
	return false;
}

struct xr_allocator
{
	template <typename T> struct helper
	{
		typedef xalloc<T> result;
	};

	static void* alloc(const u32& n)
	{
		return xr_malloc(n);
	}

	template <typename T> static void dealloc(T*& p)
	{
		xr_free(p);
	}
};

namespace std
{
template <class Tp1, class Tp2> inline xalloc<Tp2>& __stl_alloc_rebind(xalloc<Tp1>& a, const Tp2*)
{
	return reinterpret_cast<xalloc<Tp2>&>(a);
}

template <class Tp1, class Tp2> inline xalloc<Tp2> __stl_alloc_create(xalloc<Tp1>&, const Tp2*)
{
	return xalloc<Tp2>();
}
} // namespace std

// Тип строки с нашим аллокатором
typedef std::basic_string<char, std::char_traits<char>, xalloc<char>> xr_string;

// Контейнеры, наследующие стандартные и расширяющие их функциональность

// ---------- vector ----------
template <typename T, typename Alloc = xalloc<T>> class xr_vector : public std::vector<T, Alloc>
{
  private:
	using inherited = std::vector<T, Alloc>;

  public:
	using typename inherited::allocator_type;
	using typename inherited::value_type;
	using typename inherited::size_type;
	using typename inherited::reference;
	using typename inherited::const_reference;

	xr_vector() = default;
	xr_vector(size_t count, const T& value) : inherited(count, value)
	{
	}
	explicit xr_vector(size_t count) : inherited(count)
	{
	}

	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}

	void clear_and_free()
	{
		inherited::clear();
	}

	void clear_not_free()
	{
		erase(begin(), end());
	}

	void clear_and_reserve()
	{
		if (capacity() <= (size() + size() / 4))
			clear_not_free();
		else
		{
			u32 old_size = size();
			clear_and_free();
			reserve(old_size);
		}
	}

#ifdef M_DONTDEFERCLEAR_EXT
	void clear()
	{
		clear_and_free();
	}
#else
	void clear()
	{
		clear_not_free();
	}
#endif

	const_reference operator[](size_type pos) const
	{
		VERIFY(pos < size());
		return *(begin() + pos);
	}

	reference operator[](size_type pos)
	{
		VERIFY(pos < size());
		return *(begin() + pos);
	}
};

// Частичная специализация для bool
template <> class xr_vector<bool, xalloc<bool>> : public std::vector<bool, xalloc<bool>>
{
  private:
	using inherited = std::vector<bool, xalloc<bool>>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
	void clear()
	{
		erase(begin(), end());
	}
};

template <typename Alloc> class xr_vector<bool, Alloc> : public std::vector<bool, Alloc>
{
  private:
	using inherited = std::vector<bool, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
	void clear()
	{
		erase(begin(), end());
	}
};

// ---------- deque ----------
template <typename T, typename Alloc = xalloc<T>> class xr_deque : public std::deque<T, Alloc>
{
  private:
	using inherited = std::deque<T, Alloc>;

  public:
	using typename inherited::allocator_type;
	using typename inherited::value_type;
	using typename inherited::size_type;

	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

// ---------- stack ----------
template <typename T, typename Container = xr_vector<T>> class xr_stack
{
  public:
	using container_type = Container;
	using value_type = typename Container::value_type;
	using size_type = typename Container::size_type;
	using allocator_type = typename Container::allocator_type;

	allocator_type get_allocator() const
	{
		return c.get_allocator();
	}
	bool empty() const
	{
		return c.empty();
	}
	u32 size() const
	{
		return static_cast<u32>(c.size());
	}
	value_type& top()
	{
		return c.back();
	}
	const value_type& top() const
	{
		return c.back();
	}
	void push(const value_type& x)
	{
		c.push_back(x);
	}
	void pop()
	{
		c.pop_back();
	}

	bool operator==(const xr_stack& x) const
	{
		return c == x.c;
	}
	bool operator!=(const xr_stack& x) const
	{
		return !(*this == x);
	}
	bool operator<(const xr_stack& x) const
	{
		return c < x.c;
	}
	bool operator>(const xr_stack& x) const
	{
		return x < *this;
	}
	bool operator<=(const xr_stack& x) const
	{
		return !(x < *this);
	}
	bool operator>=(const xr_stack& x) const
	{
		return !(*this < x);
	}

  protected:
	Container c;
};

// ---------- list ----------
template <typename T, typename Alloc = xalloc<T>> class xr_list : public std::list<T, Alloc>
{
  private:
	using inherited = std::list<T, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

// ---------- set / multiset ----------
template <typename K, typename Pred = std::less<K>, typename Alloc = xalloc<K>>
class xr_set : public std::set<K, Pred, Alloc>
{
  private:
	using inherited = std::set<K, Pred, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

template <typename K, typename Pred = std::less<K>, typename Alloc = xalloc<K>>
class xr_multiset : public std::multiset<K, Pred, Alloc>
{
  private:
	using inherited = std::multiset<K, Pred, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

// ---------- map / multimap ----------
template <typename K, typename V, typename Pred = std::less<K>, typename Alloc = xalloc<std::pair<const K, V>>>
class xr_map : public std::map<K, V, Pred, Alloc>
{
  private:
	using inherited = std::map<K, V, Pred, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

template <typename K, typename V, typename Pred = std::less<K>, typename Alloc = xalloc<std::pair<const K, V>>>
class xr_multimap : public std::multimap<K, V, Pred, Alloc>
{
  private:
	using inherited = std::multimap<K, V, Pred, Alloc>;

  public:
	u32 size() const
	{
		return static_cast<u32>(inherited::size());
	}
};

template <class Ty1, class Ty2> inline std::pair<Ty1, Ty2> mk_pair(Ty1 val1, Ty2 val2)
{
	return std::pair<Ty1, Ty2>(val1, val2);
}

// Функторы сравнения строк
struct pred_str
{
	IC bool operator()(const char* x, const char* y) const
	{
		return xr_strcmp(x, y) < 0;
	}
};

struct pred_stri
{
	IC bool operator()(const char* x, const char* y) const
	{
		return xr_stricmp(x, y) < 0;
	}
};

// Макросы для удобного объявления типов контейнеров
#define DEF_VECTOR(N, T)                                                                                               \
	typedef xr_vector<T> N;                                                                                            \
	typedef N::iterator N##_it;
#define DEF_LIST(N, T)                                                                                                 \
	typedef xr_list<T> N;                                                                                              \
	typedef N::iterator N##_it;
#define DEF_DEQUE(N, T)                                                                                                \
	typedef xr_deque<T> N;                                                                                             \
	typedef N::iterator N##_it;
#define DEF_MAP(N, K, T)                                                                                               \
	typedef xr_map<K, T> N;                                                                                            \
	typedef N::iterator N##_it;

#define DEFINE_VECTOR(T, N, I)                                                                                         \
	typedef xr_vector<T> N;                                                                                            \
	typedef N::iterator I;
#define DEFINE_LIST(T, N, I)                                                                                           \
	typedef xr_list<T> N;                                                                                              \
	typedef N::iterator I;
#define DEFINE_DEQUE(T, N, I)                                                                                          \
	typedef xr_deque<T> N;                                                                                             \
	typedef N::iterator I;
#define DEFINE_MAP(K, T, N, I)                                                                                         \
	typedef xr_map<K, T> N;                                                                                            \
	typedef N::iterator I;
#define DEFINE_MAP_PRED(K, T, N, I, P)                                                                                 \
	typedef xr_map<K, T, P> N;                                                                                         \
	typedef N::iterator I;
#define DEFINE_MMAP(K, T, N, I)                                                                                        \
	typedef xr_multimap<K, T> N;                                                                                       \
	typedef N::iterator I;
#define DEFINE_SVECTOR(T, C, N, I)                                                                                     \
	typedef svector<T, C> N;                                                                                           \
	typedef N::iterator I;
#define DEFINE_SET(T, N, I)                                                                                            \
	typedef xr_set<T> N;                                                                                               \
	typedef N::iterator I;
#define DEFINE_SET_PRED(T, N, I, P)                                                                                    \
	typedef xr_set<T, P> N;                                                                                            \
	typedef N::iterator I;
#define DEFINE_STACK(T, N) typedef xr_stack<T> N;

// Подключение дополнительных контейнеров
#include "FixedVector.h"
#include "buffer_vector.h"

// Стандартные определения типов
DEFINE_VECTOR(bool, boolVec, boolIt);
DEFINE_VECTOR(BOOL, BOOLVec, BOOLIt);
DEFINE_VECTOR(BOOL*, LPBOOLVec, LPBOOLIt);
DEFINE_VECTOR(Frect, FrectVec, FrectIt);
DEFINE_VECTOR(Irect, IrectVec, IrectIt);
DEFINE_VECTOR(Fplane, PlaneVec, PlaneIt);
DEFINE_VECTOR(fvec2, Fvector2Vec, Fvector2It);
DEFINE_VECTOR(fvec3, FvectorVec, FvectorIt);
DEFINE_VECTOR(fvec3*, LPFvectorVec, LPFvectorIt);
DEFINE_VECTOR(Fcolor, FcolorVec, FcolorIt);
DEFINE_VECTOR(Fcolor*, LPFcolorVec, LPFcolorIt);
DEFINE_VECTOR(LPSTR, LPSTRVec, LPSTRIt);
DEFINE_VECTOR(LPCSTR, LPCSTRVec, LPCSTRIt);
DEFINE_VECTOR(xr_string, SStringVec, SStringVecIt);

DEFINE_VECTOR(s8, S8Vec, S8It);
DEFINE_VECTOR(s8*, LPS8Vec, LPS8It);
DEFINE_VECTOR(s16, S16Vec, S16It);
DEFINE_VECTOR(s16*, LPS16Vec, LPS16It);
DEFINE_VECTOR(s32, S32Vec, S32It);
DEFINE_VECTOR(s32*, LPS32Vec, LPS32It);
DEFINE_VECTOR(u8, U8Vec, U8It);
DEFINE_VECTOR(u8*, LPU8Vec, LPU8It);
DEFINE_VECTOR(u16, U16Vec, U16It);
DEFINE_VECTOR(u16*, LPU16Vec, LPU16It);
DEFINE_VECTOR(u32, U32Vec, U32It);
DEFINE_VECTOR(u32*, LPU32Vec, LPU32It);
DEFINE_VECTOR(float, FloatVec, FloatIt);
DEFINE_VECTOR(float*, LPFloatVec, LPFloatIt);
DEFINE_VECTOR(int, IntVec, IntIt);
DEFINE_VECTOR(int*, LPIntVec, LPIntIt);
