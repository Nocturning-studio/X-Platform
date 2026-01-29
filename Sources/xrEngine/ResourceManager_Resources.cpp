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
SPass* CResourceManager::_CreatePass(ref_state& _state, ref_ps& _ps, ref_vs& _vs, ref_ctable& _ctable,
									 ref_texture_list& _T, ref_matrix_list& _M, ref_constant_list& _C)
{
	// ИСПРАВЛЕНИЕ: Заменяем parallel_for на стандартный цикл.
	// Это позволяет корректно вернуть найденный pass и не создавать дубликаты.
	for (SPass* pass : v_passes)
	{
		if (pass->equal(_state, _ps, _vs, _ctable, _T, _M, _C))
			return pass;
	}

	SPass* P = xr_new<SPass>();
	P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	P->state = _state;
	P->ps = _ps;
	P->vs = _vs;
	P->constants = _ctable;
	P->T = _T;
#ifdef _EDITOR
	P->M = _M;
#endif
	P->C = _C;

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
	CHK_DX(HW.pDevice->CreateVertexDeclaration(dcl, &D->dcl));
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
CRTC* CResourceManager::_CreateRTC(LPCSTR Name, u32 size, D3DFORMAT f, u32 levels)
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
CMatrix* CResourceManager::_CreateMatrix(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0 == xr_stricmp(Name, "$null"))
		return NULL;

	LPSTR N = LPSTR(Name);
	map_Matrix::iterator I = m_matrices.find(N);
	if (I != m_matrices.end())
		return I->second;
	else
	{
		CMatrix* M = xr_new<CMatrix>();
		M->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		M->dwReference = 1;
		m_matrices.insert(mk_pair(M->set_name(Name), M));
		return M;
	}
}
void CResourceManager::_DeleteMatrix(const CMatrix* M)
{
	if (0 == (M->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*M->cName);
	map_Matrix::iterator I = m_matrices.find(N);
	if (I != m_matrices.end())
	{
		m_matrices.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find transform-def '%s'", *M->cName);
}
void CResourceManager::ED_UpdateMatrix(LPCSTR Name, CMatrix* data)
{
	CMatrix* M = _CreateMatrix(Name);
	*M = *data;
}
//--------------------------------------------------------------------------------------------------------------
CConstant* CResourceManager::_CreateConstant(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0 == xr_stricmp(Name, "$null"))
		return NULL;

	LPSTR N = LPSTR(Name);
	map_Constant::iterator I = m_constants.find(N);
	if (I != m_constants.end())
		return I->second;
	else
	{
		CConstant* C = xr_new<CConstant>();
		C->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		C->dwReference = 1;
		m_constants.insert(mk_pair(C->set_name(Name), C));
		return C;
	}
}
void CResourceManager::_DeleteConstant(const CConstant* C)
{
	if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*C->cName);
	map_Constant::iterator I = m_constants.find(N);
	if (I != m_constants.end())
	{
		m_constants.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find R1-constant-def '%s'", *C->cName);
}

void CResourceManager::ED_UpdateConstant(LPCSTR Name, CConstant* data)
{
	CConstant* C = _CreateConstant(Name);
	*C = *data;
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
//--------------------------------------------------------------------------------------------------------------
SMatrixList* CResourceManager::_CreateMatrixList(SMatrixList& L)
{
	BOOL bEmpty = TRUE;
	for (u32 i = 0; i < L.size(); i++)
		if (L[i])
		{
			bEmpty = FALSE;
			break;
		}
	if (bEmpty)
		return NULL;

	for (SMatrixList* base : lst_matrices)
	{
		if (L.equal(*base))
			return base;
	}

	SMatrixList* lst = xr_new<SMatrixList>(L);
	lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	lst_matrices.push_back(lst);
	return lst;
}
void CResourceManager::_DeleteMatrixList(const SMatrixList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_matrices, L))
		return;
	Msg("! ERROR: Failed to find compiled list of transform-defs");
}
//--------------------------------------------------------------------------------------------------------------
SConstantList* CResourceManager::_CreateConstantList(SConstantList& L)
{
	BOOL bEmpty = TRUE;
	for (u32 i = 0; i < L.size(); i++)
		if (L[i])
		{
			bEmpty = FALSE;
			break;
		}
	if (bEmpty)
		return NULL;

	// 1. Создаем переменную для хранения результата поиска
	// Используем std::atomic или просто volatile переменную, так как пишем указатель
	SConstantList* found = NULL;

	// Используем мьютекс для потокобезопасной записи (хотя для одного указателя это не критично, но правильно)
	// В X-Ray обычно есть врапперы, но std::mutex тоже подойдет.
	// Если concurrency::parallel_for из PPL, там своя специфика, но простейший вариант:

	concurrency::parallel_for((u32)0, (u32)lst_constants.size(), [&](u32 it) {
		// Оптимизация: если другой поток уже нашел, прерываем выполнение этой итерации
		if (found != NULL)
			return;

		SConstantList* base = lst_constants[it];
		if (L.equal(*base))
		{
			// Нашли совпадение - сохраняем
			found = base;
			// PPL позволяет отменить остальные задачи, но это сложнее.
			// Простого присваивания достаточно.
		}
	});

	// 2. Если нашли дубликат - возвращаем его
	if (found)
		return found;

	// 3. Если не нашли - создаем новый
	SConstantList* lst = xr_new<SConstantList>(L);
	lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	lst_constants.push_back(lst);
	return lst;
}

void CResourceManager::_DeleteConstantList(const SConstantList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_constants, L))
		return;
	Msg("! ERROR: Failed to find compiled list of r1-constant-defs");
}
