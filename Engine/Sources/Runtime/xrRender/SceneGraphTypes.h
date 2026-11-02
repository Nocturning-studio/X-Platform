#pragma once

#include <map>
#include <functional>
#include <xrCore/FixedMap.h>
#include "doug_lea_allocator_wrapper.h"
#include "LightTrack.h"
#include "SceneGraphArenaAllocator.h"

// #define USE_RESOURCE_DEBUGGER

namespace SceneGraphTypes
{
////////////////////////////////////////////////////////////////////////////////
//  Elementary Types (Nodes)
////////////////////////////////////////////////////////////////////////////////

struct StaticRenderNode
{
	float screenSpaceArea;
	IRender_Visual* pVisual;
};

struct DynamicRenderNode
{
	float screenSpaceArea;
	IRender_Visual* pVisual;
	fmat4x4 transform;
	CROS_impl::AOCube ao_cube;

	// Дефолтный — чтобы ноды в контейнерах не содержали мусора
	// до того, как их заполнят через Copy()/конструктор.
	DynamicRenderNode() : screenSpaceArea(0.f), pVisual(nullptr)
	{
		transform.identity();
		ZeroMemory(ao_cube.data(), sizeof(ao_cube));
	}

	// Конструктор "на месте" — то, что чаще всего нужно
	// в EnqueueDynamic / EnqueueStatic.
	DynamicRenderNode(float ssa, IRender_Visual* pVis, const fmat4x4& trans, const float* hcube) : screenSpaceArea(ssa), pVisual(pVis), transform(trans)
	{
		CopyMemory(ao_cube.data(), hcube, sizeof(ao_cube));
	}

	// Универсальный заполнитель, если нода уже создана
	// (например, в FixedMAP вернулась из insert()).
	IC void Copy(float ssa, IRender_Visual* pVis, const fmat4x4& trans, const float* hcube)
	{
		screenSpaceArea = ssa;
		pVisual = pVis;
		transform = trans;
		CopyMemory(ao_cube.data(), hcube, sizeof(ao_cube));
	}
};

struct _MatrixItemS : public DynamicRenderNode
{
	ShaderElement* se = nullptr;
};

struct LodRenderNode
{
	float screenSpaceArea;
	IRender_Visual* pVisual;
};

////////////////////////////////////////////////////////////////////////////////
//  Shader Types Aliases
////////////////////////////////////////////////////////////////////////////////
#ifdef USE_RESOURCE_DEBUGGER
using vs_type = ref_vs;
using ps_type = ref_ps;
#else
using vs_type = IDirect3DVertexShader9*;
using ps_type = IDirect3DPixelShader9*;
#endif

////////////////////////////////////////////////////////////////////////////////
//  Hierarchical Maps Definitions
//  Hierarchy: VS -> PS -> CS -> States -> Textures -> Items
////////////////////////////////////////////////////////////////////////////////
struct PassKey
{
	vs_type vs;
	ps_type ps;
	R_constant_table* constants;
	IDirect3DStateBlock9* state;
	STextureList* textures;

	bool operator<(const PassKey& o) const noexcept
	{
		if(vs != o.vs)
			return std::less<vs_type>()(vs, o.vs);
		if(ps != o.ps)
			return std::less<ps_type>()(ps, o.ps);
		if(constants != o.constants)
			return std::less<R_constant_table*>()(constants, o.constants);
		if(state != o.state)
			return std::less<IDirect3DStateBlock9*>()(state, o.state);
		return std::less<STextureList*>()(textures, o.textures);
	}
	bool operator==(const PassKey& o) const noexcept
	{
		return vs == o.vs && ps == o.ps && constants == o.constants && state == o.state && textures == o.textures;
	}
	bool operator!=(const PassKey& o) const noexcept { return !(*this == o); }
};

template <typename TNode>
struct RenderEntry
{
	SceneGraphTypes::PassKey key;
	TNode item;
};

template <typename TNode>
struct PassBatch
{
	SceneGraphTypes::PassKey key;
	u32 first = 0; // индекс в entries
	u32 count = 0;
	float max_ssa = 0.f;
};

template <typename TNode>
struct RenderQueue
{
	using AllocEntry = ArenaAlloc<RenderEntry<TNode>>;
	using AllocBatch = ArenaAlloc<PassBatch<TNode>>;

	xr_vector<RenderEntry<TNode>, AllocEntry> entries;
	xr_vector<PassBatch<TNode>, AllocBatch> batches;

	SceneGraphArena* arena = nullptr;

	void Init(SceneGraphArena* a)
	{
		arena = a;

		std::vector<RenderEntry<TNode>, AllocEntry> tmp_e((AllocEntry(a)));
		std::vector<PassBatch<TNode>, AllocBatch> tmp_b((AllocBatch(a)));

		entries.swap(tmp_e);
		batches.swap(tmp_b);
	}

	void Clear()
	{
		entries.clear();
		batches.clear();
	}

	void ReserveAfterReset(u32 entry_hint, u32 batch_hint)
	{
		if (!arena)
			return;

		std::vector<RenderEntry<TNode>, AllocEntry> new_e((AllocEntry(arena)));
		std::vector<PassBatch<TNode>, AllocBatch> new_b((AllocBatch(arena)));

		entries.swap(new_e);
		batches.swap(new_b);

		entries.reserve(entry_hint);
		batches.reserve(batch_hint);
	}

	void SortAndGroup()
	{
		if(entries.empty())
		{
			batches.clear();
			return;
		}

		std::sort(entries.begin(), entries.end(),
				  [](const RenderEntry<TNode>& a, const RenderEntry<TNode>& b)
				  {
					  return a.key < b.key;
				  });

		batches.clear();
		batches.push_back({entries[0].key, 0, 1, entries[0].item.screenSpaceArea});

		for(u32 i = 1; i < entries.size(); ++i)
		{
			PassBatch<TNode>& b = batches.back();
			if(entries[i].key != b.key)
			{
				batches.push_back({entries[i].key, i, 1, entries[i].item.screenSpaceArea});
			}
			else
			{
				b.count++;
				if(entries[i].item.screenSpaceArea > b.max_ssa)
					b.max_ssa = entries[i].item.screenSpaceArea;
			}
		}
	}

	[[nodiscard]] bool IsEmpty() const noexcept { return entries.empty(); }
	[[nodiscard]] u32  Size() const noexcept { return entries.size(); }
};

template <typename TNode>
struct SortedQueue
{
	using Entry = std::pair<float, TNode>;
	using Alloc = ArenaAlloc<Entry>;

	std::vector<Entry, Alloc> entries;
	SceneGraphArena* arena = nullptr;

	void Init(SceneGraphArena* a)
	{
		arena = a;
		std::vector<Entry, Alloc> tmp((Alloc(a)));
		entries.swap(tmp);
	}

	void Clear() { entries.clear(); }

	void ReserveAfterReset(u32 hint)
	{
		if (!arena) return;
		std::vector<Entry, Alloc> tmp((Alloc(arena)));
		entries.swap(tmp);
		entries.reserve(hint);
	}

	void Push(float key, const TNode& item) { entries.push_back({ key, item }); }

	// front-to-back (ascending distance)
	void SortLR() 
	{
		std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.first < b.first; });
	}
	void SortRL() 
	{
		std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.first > b.first; });
	}

	u32 Size()  const { return entries.size(); }
	bool IsEmpty() const { return entries.empty(); }
};

using QueueTransparent = SortedQueue<_MatrixItemS>;
using QueueHUD = SortedQueue<_MatrixItemS>;
using QueueLOD = SortedQueue<LodRenderNode>;

}; // namespace SceneGraphTypes
