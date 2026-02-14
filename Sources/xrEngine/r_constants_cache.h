#ifndef r_constants_cacheH
#define r_constants_cacheH
#pragma once

#include "r_constants.h"
#include "device.h" // Òåïåğü Device äîñòóïåí

template <class T, u32 limit> class R_constant_cache
{
  private:
	ALIGN(16) svector<T, limit> array;
	u32 lo, hi;

  public:
	R_constant_cache()
	{
		array.resize(limit);
		flush();
	}

	ICF T* access(u32 id)
	{
		return &array[id];
	}
	ICF void flush()
	{
		lo = hi = 0;
	}
	ICF void dirty(u32 _lo, u32 _hi)
	{
		if (_lo < lo)
			lo = _lo;
		if (_hi > hi)
			hi = _hi;
	}
	ICF u32 r_lo()
	{
		return lo;
	}
	ICF u32 r_hi()
	{
		return hi;
	}
};

class ENGINE_API R_constant_array
{
  public:
	typedef R_constant_cache<float4, 256> t_f;

  public:
	ALIGN(16) t_f c_f;
	BOOL b_dirty;

  private:
	// ÄÎÁÀÂËßÅÌ: Dirty-ñèñòåìà
	u32 m_dirtyFrame;
	u32 m_lastFlushFrame;

	u32 getFrame();

  public:
	// ÄÎÁÀÂËßÅÌ: Êîíñòğóêòîğ äëÿ èíèöèàëèçàöèè
	R_constant_array() : b_dirty(FALSE), m_dirtyFrame(0), m_lastFlushFrame(0)
	{
	}

	// ÄÎÁÀÂËßÅÌ: Dirty-ìåòîäû
	ICF void mark_dirty()
	{
		m_dirtyFrame = getFrame();
		b_dirty = TRUE;
	}

	ICF bool needs_flush() const
	{
		// ÈÑÏĞÀÂËÅÍÈÅ: ïğîâåğÿåì ÷òî dirty frame áîëüøå last flush frame
		return b_dirty && (m_dirtyFrame > m_lastFlushFrame);
	}

	ICF void mark_flushed()
	{
		m_lastFlushFrame = getFrame();
		b_dirty = FALSE;
	}

	ICF void reset_dirty()
	{
		m_dirtyFrame = 0;
		m_lastFlushFrame = 0;
		b_dirty = FALSE;
	}

	ICF void force_dirty()
	{
		mark_dirty();
	}

	t_f& get_array_f()
	{
		return c_f;
	}

	// ÌÎÄÈÔÈÖÈĞÓÅÌ ñóùåñòâóşùèå ìåòîäû - äîáàâëÿåì mark_dirty()
	void set(R_constant* C, R_constant_load& L, const float4x4& A)
	{
		VERIFY(RC_float == C->type);
		float4* it = c_f.access(L.index);
		switch (L.cls)
		{
		case RC_2x4:
			c_f.dirty(L.index, L.index + 2);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			break;
		case RC_3x4:
			c_f.dirty(L.index, L.index + 3);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			break;
		case RC_4x4:
			c_f.dirty(L.index, L.index + 4);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			break;
		default:
#ifdef DEBUG
			Debug.fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
		mark_dirty();
	}

	void set(R_constant* C, R_constant_load& L, const float4& A)
	{
		VERIFY(RC_float == C->type);
		VERIFY(RC_1x4 == L.cls);
		c_f.access(L.index)->set(A);
		c_f.dirty(L.index, L.index + 1);
		mark_dirty();
	}

	void seta(R_constant* C, R_constant_load& L, u32 e, const float4x4& A)
	{
		VERIFY(RC_float == C->type);
		u32 base;
		float4* it;
		switch (L.cls)
		{
		case RC_2x4:
			base = L.index + 2 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 2);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			break;
		case RC_3x4:
			base = L.index + 3 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 3);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			break;
		case RC_4x4:
			base = L.index + 4 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 4);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			break;
		default:
#ifdef DEBUG
			Debug.fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
		mark_dirty();
	}

	void seta(R_constant* C, R_constant_load& L, u32 e, const float4& A)
	{
		VERIFY(RC_float == C->type);
		VERIFY(RC_1x4 == L.cls);
		u32 base = L.index + e;
		c_f.access(base)->set(A);
		c_f.dirty(base, base + 1);
		mark_dirty();
	}
};

class ENGINE_API R_constants
{
  public:
	ALIGN(16) R_constant_array a_pixel;
	ALIGN(16) R_constant_array a_vertex;

	// ÄÎÁÀÂËßÅÌ: Êîíñòğóêòîğ
	R_constants()
	{
	}

	void flush_cache();

  public:
	// ÂĞÅÌÅÍÍÎ ÂÎÇÂĞÀÙÀÅÌ ñòàğóş ëîãèêó äëÿ îòëàäêè
	ICF void set(R_constant* C, const float4x4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.set(C, C->ps, A);
			a_pixel.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
		if (C->destination & 2)
		{
			a_vertex.set(C, C->vs, A);
			a_vertex.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
	}

	ICF void set(R_constant* C, const float4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.set(C, C->ps, A);
			a_pixel.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
		if (C->destination & 2)
		{
			a_vertex.set(C, C->vs, A);
			a_vertex.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
	}

	ICF void set(R_constant* C, float x, float y, float z, float w)
	{
		float4 data;
		data.set(x, y, z, w);
		set(C, data);
	}

	ICF void seta(R_constant* C, u32 e, const float4x4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.seta(C, C->ps, e, A);
			a_pixel.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
		if (C->destination & 2)
		{
			a_vertex.seta(C, C->vs, e, A);
			a_vertex.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
	}

	ICF void seta(R_constant* C, u32 e, const float4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.seta(C, C->ps, e, A);
			a_pixel.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
		if (C->destination & 2)
		{
			a_vertex.seta(C, C->vs, e, A);
			a_vertex.b_dirty = TRUE; // ÂĞÅÌÅÍÍÎ ÎÑÒÀÂËßÅÌ
		}
	}

	ICF void seta(R_constant* C, u32 e, float x, float y, float z, float w)
	{
		float4 data;
		data.set(x, y, z, w);
		seta(C, e, data);
	}

	ICF void flush()
	{
		if (a_pixel.b_dirty || a_vertex.b_dirty)
			flush_cache();
	}

	ICF void force_dirty()
	{
		a_pixel.force_dirty();
		a_vertex.force_dirty();
	}

	ICF void reset_dirty()
	{
		a_pixel.reset_dirty();
		a_vertex.reset_dirty();
	}
};
#endif