#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#ifndef _EDITOR
#include "render.h"
#endif
#pragma warning(default : 4995)

#include "ResourceManager.h"
#include "tss.h"
#include "blender.h"
#include "blender_recorder.h"

#include "ShaderResourceTraits.h"
#include "ShaderCompile.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)

//--------------------------------------------------------------------------------------------------------------
SState* CResourceManager::_CreateState(SimulatorStates& state_code)
{
	for (SState* C : v_states)
	{
		SimulatorStates& base = C->state_code;

		if (base.equal(state_code))
			return C;
	}

	// Create New
	v_states.push_back(xr_new<SState>());
	v_states.back()->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_states.back()->state = state_code.record();
	v_states.back()->state_code = state_code;
	return v_states.back();
}

void CResourceManager::_DeleteState(const SState* state)
{
	if (0 == (state->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	if (reclaim(v_states, state))
		return;

	Msg("! ERROR: Failed to find compiled stateblock");
}

//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(ref_state& _state, ref_ps& _ps, ref_vs& _vs, ref_ctable& _ctable, ref_texture_list& _T)
{
	for (SPass* pass : v_passes)
	{
		if (pass->equal(_state, _ps, _vs, _ctable, _T))
			return pass;
	}

	SPass* P = xr_new<SPass>();
	P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	P->state = _state;
	P->ps = _ps;
	P->vs = _vs;
	P->constants = _ctable;
	P->T = _T;

	v_passes.push_back(P);

	return v_passes.back();
}

void CResourceManager::_DeletePass(const SPass* P)
{
	if (0 == (P->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	if (reclaim(v_passes, P))
		return;

	Msg("! ERROR: Failed to find compiled pass");
}

//--------------------------------------------------------------------------------------------------------------
static BOOL dcl_equal(D3DVERTEXELEMENT9* a, D3DVERTEXELEMENT9* b)
{
	// check sizes
	u32 a_size = D3DXGetDeclLength(a);
	u32 b_size = D3DXGetDeclLength(b);

	if (a_size != b_size)
		return FALSE;

	return 0 == memcmp(a, b, a_size * sizeof(D3DVERTEXELEMENT9));
}

SDeclaration* CResourceManager::_CreateDecl(D3DVERTEXELEMENT9* dcl)
{
	for (SDeclaration* D : v_declarations)
	{
		if (dcl_equal(dcl, &*D->dcl_code.begin()))
			return D;
	}

	// Create _new
	SDeclaration* D = xr_new<SDeclaration>();
	u32 dcl_size = D3DXGetDeclLength(dcl) + 1;
	CHK_DX(HW.GetDevice()->CreateVertexDeclaration(dcl, &D->dcl));
	D->dcl_code.assign(dcl, dcl + dcl_size);
	D->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_declarations.push_back(D);
	return D;
}

void CResourceManager::_DeleteDecl(const SDeclaration* dcl)
{
	if (0 == (dcl->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_declarations, dcl))
		return;
	Msg("! ERROR: Failed to find compiled vertex-declarator");
}
//--------------------------------------------------------------------------------------------------------------

R_constant_table* CResourceManager::_CreateConstantTable(R_constant_table& C)
{
	if (C.empty())
		return NULL;

	for (R_constant_table* table : v_constant_tables)
	{
		if (table->equal(C))
			return table;
	}

	v_constant_tables.push_back(xr_new<R_constant_table>(C));
	v_constant_tables.back()->dwFlags |= xr_resource_flagged::RF_REGISTERED;

	return v_constant_tables.back();
}

void CResourceManager::_DeleteConstantTable(const R_constant_table* C)
{
	if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	if (reclaim(v_constant_tables, C))
		return;

	Msg("! ERROR: Failed to find compiled constant-table");
}

//--------------------------------------------------------------------------------------------------------------
CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, xrRHI::RHI_Format f, u32 levels)
{
	R_ASSERT(Name && Name[0] && w && h);

	// ***** first pass - search already created RT
	LPSTR N = LPSTR(Name);
	map_RT::iterator I = m_rtargets.find(N);

	if (I != m_rtargets.end())
	{
		return I->second;
	}
	else
	{
		CRT* RT = xr_new<CRT>();
		RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		m_rtargets.insert(mk_pair(RT->set_name(Name), RT));

		if (Device.b_is_Ready)
			RT->create(Name, w, h, f, levels);

		return RT;
	}
}

void CResourceManager::_DeleteRT(const CRT* RT)
{
	if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*RT->cName);
	map_RT::iterator I = m_rtargets.find(N);
	if (I != m_rtargets.end())
	{
		m_rtargets.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}
//--------------------------------------------------------------------------------------------------------------
CRTC* CResourceManager::_CreateRTC(LPCSTR Name, u32 size, xrRHI::RHI_Format f, u32 levels)
{
	R_ASSERT(Name && Name[0] && size);

	// ***** first pass - search already created RTC
	LPSTR N = LPSTR(Name);
	map_RTC::iterator I = m_rtargets_c.find(N);

	if (I != m_rtargets_c.end())
	{
		return I->second;
	}
	else
	{
		CRTC* RT = xr_new<CRTC>();
		RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		m_rtargets_c.insert(mk_pair(RT->set_name(Name), RT));

		if (Device.b_is_Ready)
			RT->create(Name, size, f, levels);

		return RT;
	}
}

void CResourceManager::_DeleteRTC(const CRTC* RT)
{
	if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	LPSTR N = LPSTR(*RT->cName);
	map_RTC::iterator I = m_rtargets_c.find(N);
	if (I != m_rtargets_c.end())
	{
		m_rtargets_c.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}
//--------------------------------------------------------------------------------------------------------------

SGeometry* CResourceManager::CreateGeom(D3DVERTEXELEMENT9* decl, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib)
{
	R_ASSERT(decl && vb);

	SDeclaration* dcl = _CreateDecl(decl);
	u32 vb_stride = D3DXGetDeclVertexSize(decl, 0);

	for (SGeometry* G : v_geoms)
	{
		if ((G->dcl == dcl) && (G->vb == vb) && (G->ib == ib) && (G->vb_stride == vb_stride))
			return G;
	}

	// Если не нашли - создаем новый
	SGeometry* Geom = xr_new<SGeometry>();
	Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	Geom->dcl = dcl;
	Geom->vb = vb;
	Geom->vb_stride = vb_stride;
	Geom->ib = ib;
	v_geoms.push_back(Geom);
	return Geom;
}

SGeometry* CResourceManager::CreateGeom(u32 FVF, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib)
{
	D3DVERTEXELEMENT9 dcl[MAX_FVF_DECL_SIZE];
	CHK_DX(D3DXDeclaratorFromFVF(FVF, dcl));
	SGeometry* g = CreateGeom(dcl, vb, ib);
	return g;
}

void CResourceManager::DeleteGeom(const SGeometry* Geom)
{
	if (this == NULL)
		return;

	if (0 == (Geom->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	if (reclaim(v_geoms, Geom))
		return;

	Msg("! ERROR: Failed to find compiled geometry-declaration");
}

//--------------------------------------------------------------------------------------------------------------
#pragma todo("NSDeathman to NSDeathman: Вынести на второй поток")
CTexture* CResourceManager::_CreateTexture(LPCSTR _Name)
{
	if (0 == xr_strcmp(_Name, "null"))
		return 0;

	R_ASSERT(_Name && _Name[0]);
	string_path Name;
	strcpy_s(Name, _Name);
	fix_texture_name(Name);
	// ***** first pass - search already loaded texture
	LPSTR N = LPSTR(Name);
	map_TextureIt I = m_textures.find(N);

	if (I != m_textures.end())
	{
		return I->second;
	}
	else
	{
		CTexture* T = xr_new<CTexture>();
		T->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		m_textures.insert(mk_pair(T->set_name(Name), T));
		T->Preload();

		if (Device.b_is_Ready && !bDeferredLoad)
			T->Load();

		return T;
	}
}

void CResourceManager::_DeleteTexture(const CTexture* T)
{
	if (0 == (T->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*T->cName);
	map_Texture::iterator I = m_textures.find(N);
	if (I != m_textures.end())
	{
		m_textures.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find texture surface '%s'", *T->cName);
}
//--------------------------------------------------------------------------------------------------------------
bool cmp_tl(const std::pair<u32, ref_texture>& _1, const std::pair<u32, ref_texture>& _2)
{
	return _1.first < _2.first;
}
STextureList* CResourceManager::_CreateTextureList(STextureList& L)
{
	std::sort(L.begin(), L.end(), cmp_tl);

	for (STextureList* base : lst_textures)
	{
		if (L.equal(*base))
			return base;
	}

	// 3. Создание нового.
	STextureList* lst = xr_new<STextureList>(L);
	lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	lst_textures.push_back(lst);
	return lst;
}
void CResourceManager::_DeleteTextureList(const STextureList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_textures, L))
		return;
	Msg("! ERROR: Failed to find compiled list of textures");
}

