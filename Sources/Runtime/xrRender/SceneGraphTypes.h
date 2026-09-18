#pragma once

#include <xrCore/FixedMap.h>
#include "doug_lea_allocator_wrapper.h"
#include "LightTrack.h"

// #define USE_RESOURCE_DEBUGGER

namespace SceneGraphTypes
{
// =========================================================================
//  Elementary Types (Nodes)
// =========================================================================

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

	// ƒефолтный Ч чтобы ноды в контейнерах не содержали мусора
	// до того, как их заполн€т через Copy()/конструктор.
	DynamicRenderNode()
		: screenSpaceArea(0.f), pVisual(nullptr)
	{
		transform.identity();
		ZeroMemory(ao_cube.data(), sizeof(ao_cube));
	}

	//  онструктор "на месте" Ч то, что чаще всего нужно
	// в EnqueueDynamic / EnqueueStatic.
	DynamicRenderNode(float ssa, IRender_Visual* pVis, const fmat4x4& trans, const float* hcube)
		: screenSpaceArea(ssa), pVisual(pVis), transform(trans)
	{
		CopyMemory(ao_cube.data(), hcube, sizeof(ao_cube));
	}

	// ”ниверсальный заполнитель, если нода уже создана
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
	ShaderElement* se;
};

struct LodRenderNode
{
	float screenSpaceArea;
	IRender_Visual* pVisual;
};

// =========================================================================
//  Shader Types Aliases
// =========================================================================
#ifdef USE_RESOURCE_DEBUGGER
using vs_type = ref_vs;
using ps_type = ref_ps;
#else
using vs_type = IDirect3DVertexShader9*;
using ps_type = IDirect3DPixelShader9*;
#endif

// =========================================================================
//  Hierarchical Maps Definitions
//  Hierarchy: VS -> PS -> CS -> States -> Textures -> Items
// =========================================================================

// --- Normal Geometry (Static) ---
using mapNormalDirect = xr_vector<StaticRenderNode, render_allocator::helper<StaticRenderNode>::result>;

struct mapNormalItems : public mapNormalDirect
{
	float screenSpaceArea;
};

struct mapNormalTextures : public FixedMAP<STextureList*, mapNormalItems, render_allocator>
{
	float screenSpaceArea;
};

struct mapNormalStates : public FixedMAP<IDirect3DStateBlock9*, mapNormalTextures, render_allocator>
{
	float screenSpaceArea;
};

struct mapNormalCS : public FixedMAP<R_constant_table*, mapNormalStates, render_allocator>
{
	float screenSpaceArea;
};

struct mapNormalPS : public FixedMAP<ps_type, mapNormalCS, render_allocator>
{
	float screenSpaceArea;
};

struct mapNormalVS : public FixedMAP<vs_type, mapNormalPS, render_allocator>
{
};

using mapNormal_T = mapNormalVS;

// --- Matrix Geometry (Dynamic) ---
using mapMatrixDirect = xr_vector<DynamicRenderNode, render_allocator::helper<DynamicRenderNode>::result>;

struct mapMatrixItems : public mapMatrixDirect
{
	float screenSpaceArea;
};

struct mapMatrixTextures : public FixedMAP<STextureList*, mapMatrixItems, render_allocator>
{
	float screenSpaceArea;
};

struct mapMatrixStates : public FixedMAP<IDirect3DStateBlock9*, mapMatrixTextures, render_allocator>
{
	float screenSpaceArea;
};

struct mapMatrixCS : public FixedMAP<R_constant_table*, mapMatrixStates, render_allocator>
{
	float screenSpaceArea;
};

struct mapMatrixPS : public FixedMAP<ps_type, mapMatrixCS, render_allocator>
{
	float screenSpaceArea;
};

struct mapMatrixVS : public FixedMAP<vs_type, mapMatrixPS, render_allocator>
{
};

using mapMatrix_T = mapMatrixVS;

// =========================================================================
//  Top Level Sorted Maps
// =========================================================================

using mapSorted_T = FixedMAP<float, _MatrixItemS, render_allocator>;
using mapSorted_Node = mapSorted_T::TNode;

using mapHUD_T = FixedMAP<float, _MatrixItemS, render_allocator>;
using mapHUD_Node = mapHUD_T::TNode;

using mapLOD_T = FixedMAP<float, LodRenderNode, render_allocator>;
using mapLOD_Node = mapLOD_T::TNode;

}; // namespace SceneGraphTypes
