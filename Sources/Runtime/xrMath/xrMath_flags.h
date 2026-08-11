#pragma once

template <class T> struct template_flags
{
  public:
	typedef T TYPE;
	typedef template_flags<T> Self;
	typedef Self& SelfRef;
	typedef const Self& SelfCRef;

  public:
	T flags;

	IC TYPE get()
	{
		return flags;
	}
	IC SelfRef zero()
	{
		flags = T(0);
		return *this;
	}
	IC SelfRef one()
	{
		flags = T(-1);
		return *this;
	}
	IC SelfRef invert()
	{
		flags = ~flags;
		return *this;
	}
	IC SelfRef invert(const Self& f)
	{
		flags = ~f.flags;
		return *this;
	}
	IC SelfRef invert(const T mask)
	{
		flags ^= mask;
		return *this;
	}
	IC SelfRef assign(const Self& f)
	{
		flags = f.flags;
		return *this;
	}
	IC SelfRef assign(const T mask)
	{
		flags = mask;
		return *this;
	}
	IC SelfRef set(const T mask, BOOL value)
	{
		if (value)
			flags |= mask;
		else
			flags &= ~mask;
		return *this;
	}
	IC BOOL is(const T mask) const
	{
		return mask == (flags & mask);
	}
	IC BOOL is_any(const T mask) const
	{
		return BOOL(flags & mask);
	}
	IC BOOL test(const T mask) const
	{
		return BOOL(flags & mask);
	}
	IC SelfRef bit_or (const T mask)
	{
		flags |= mask;
		return *this;
	}
	IC SelfRef bit_or(const Self& f, const T mask)
	{
		flags = f.flags | mask;
		return *this;
	}
	IC SelfRef bit_and(const T mask)
	{
		flags &= mask;
		return *this;
	}
	IC SelfRef bit_and(const Self& f, const T mask)
	{
		flags = f.flags & mask;
		return *this;
	}
	IC BOOL equal(const Self& f) const
	{
		return flags == f.flags;
	}
	IC BOOL equal(const Self& f, const T mask) const
	{
		return (flags & mask) == (f.flags & mask);
	}
};

typedef template_flags<u8> Flags8;
typedef template_flags<u8> flags8;
typedef template_flags<u16> Flags16;
typedef template_flags<u16> flags16;
typedef template_flags<u32> Flags32;
typedef template_flags<u32> flags32;
typedef template_flags<u64> Flags64;
typedef template_flags<u64> flags64;
