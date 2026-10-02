#include "pch.h"
#include "R_Backend_ResourceBinder.h"
#include "R_Backend.h"
#include "sh_texture.h"
#include "r_constants.h"
#include <xrRenderBackend/Resources/Shaders/ShaderPass.h>
////////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// Invalidate
// ----------------------------------------------------------------
void CBackendResourceBinder::Invalidate(CRenderBackendFacade& /*backend*/)
{
	m_state = nullptr;

	// RHI cache.
	m_ps = {};
	m_vs = {};
	m_decl = {};
	m_vb = {};
	m_ib = {};
	m_ibFormat = RHI_IndexFormat::UInt16;
	m_vbStride = 0;

	// Legacy cache.
	m_psLegacy = nullptr;
	m_vsLegacy = nullptr;
	m_declLegacy = nullptr;
	m_vbLegacy = nullptr;
	m_ibLegacy = nullptr;
	m_vbStrideLegacy = 0;

	m_owner = EStateOwner::Unknown;

	m_ctable = nullptr;
	m_T = nullptr;

	for (u32 i = 0; i < 16; ++i) m_texturesPS[i] = nullptr;
	for (u32 i = 0; i < 5; ++i) m_texturesVS[i] = nullptr;

#ifdef DEBUG
	m_psName = nullptr;
	m_vsName = nullptr;
#endif
}

// ----------------------------------------------------------------
// State block (legacy)
// ----------------------------------------------------------------
void CBackendResourceBinder::SetStates(CRenderBackendFacade& backend, IDirect3DStateBlock9* state)
{
	// TODO[E5]: state blocks не имеют аналога в RHI. В D3D12 модель PSO
	// делает их ненужными. Пока оставляем legacy-путь.
	if (m_state != state)
	{
#ifdef DEBUG
		backend.stat.states++;
#endif
		m_state = state;
		if (state)
			state->Apply();
	}
}

// ----------------------------------------------------------------
// Pixel shader
// ----------------------------------------------------------------
void CBackendResourceBinder::SetPixelShader(CRenderBackendFacade& backend, RHI_ShaderHandle ps, LPCSTR name)
{
	const bool cacheValid = (m_owner == EStateOwner::RHI && m_ps == ps);
	if (!cacheValid)
	{
		backend.stat.ps++;
		m_ps = ps;
		backend.GetRHI()->SetPixelShader(ps);
		m_owner = EStateOwner::RHI;
	}
#ifdef DEBUG
	m_psName = name;
#endif
}

// ----------------------------------------------------------------
// Vertex shader
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexShader(CRenderBackendFacade& backend, RHI_ShaderHandle vs, LPCSTR name)
{
	const bool cacheValid = (m_owner == EStateOwner::RHI && m_vs == vs);
	if (!cacheValid)
	{
		backend.stat.vs++;
		m_vs = vs;
		backend.GetRHI()->SetVertexShader(vs);
		m_owner = EStateOwner::RHI;
	}
#ifdef DEBUG
	m_vsName = name;
#endif
}

// ----------------------------------------------------------------
// Shader pass
// ----------------------------------------------------------------
void CBackendResourceBinder::SetShaderPass(CRenderBackendFacade& backend, CShaderPass* pass)
{
	if (!pass)
	{
		SetVertexShader(backend, RHI_ShaderHandle{}, nullptr);
		SetPixelShader(backend, RHI_ShaderHandle{}, nullptr);
		return;
	}

	const RHI_ShaderHandle vs = pass->GetVertexProgram().GetRHIHandle();
	const RHI_ShaderHandle ps = pass->GetPixelProgram().GetRHIHandle();

#ifdef DEBUG
	LPCSTR vsName = vs.IsValid() ? pass->GetVertexShaderFile() : nullptr;
	LPCSTR psName = ps.IsValid() ? pass->GetPixelShaderFile() : nullptr;
	SetVertexShader(backend, vs, vsName);
	SetPixelShader(backend, ps, psName);
#else
	SetVertexShader(backend, vs);
	SetPixelShader(backend, ps);
#endif

	// Константы — как в старом коде. Samplers/текстуры применяются
	// отдельно через SetTextures (и, при необходимости, pass->ApplySamplers).
	pass->ConstantBuffer().Flush(*backend.GetRHI());
}

// ----------------------------------------------------------------
// Vertex declaration
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexDeclaration(CRenderBackendFacade& backend, RHI_InputLayoutHandle decl)
{
	const bool cacheValid = (m_owner == EStateOwner::RHI && m_decl == decl);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.decl++;
#endif
		m_decl = decl;
		backend.GetRHI()->SetInputLayout(decl);
		m_owner = EStateOwner::RHI;
}
}

// ----------------------------------------------------------------
// Vertex buffer
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexBuffer(CRenderBackendFacade& backend, RHI_BufferHandle vb, u32 stride)
{
	const bool cacheValid = (m_owner == EStateOwner::RHI && m_vb == vb && m_vbStride == stride);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.vb++;
#endif
		m_vb = vb;
		m_vbStride = stride;
		backend.GetRHI()->SetVertexBuffer(0, vb, 0, stride);
		m_owner = EStateOwner::RHI;
	}
}

// ----------------------------------------------------------------
// Index buffer
// ----------------------------------------------------------------
void CBackendResourceBinder::SetIndexBuffer(CRenderBackendFacade& backend, RHI_BufferHandle ib, RHI_IndexFormat fmt)
{
	const bool cacheValid = (m_owner == EStateOwner::RHI && m_ib == ib);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.ib++;
#endif
		m_ib = ib;
		m_ibFormat = fmt;
		backend.GetRHI()->SetIndexBuffer(ib, fmt);
		m_owner = EStateOwner::RHI;
	}
}

// ----------------------------------------------------------------
// Constant table — без изменений
// ----------------------------------------------------------------
void CBackendResourceBinder::SetConstantTable(CRenderBackendFacade& backend, R_constant_table* ctable, R_transforms& transforms)
{
	if (m_ctable == ctable)
		return;

	m_ctable = ctable;
	transforms.unmap();

	if (!ctable)
		return;

	R_constant_table::c_table::iterator it = ctable->table.begin();
	R_constant_table::c_table::iterator end = ctable->table.end();
	for (; it != end; ++it)
	{
		R_constant* C = &**it;
		if (C->handler)
			C->handler->setup(C);
	}
}

// ----------------------------------------------------------------
// Textures — TODO[E3b]: пока legacy.
// ----------------------------------------------------------------
void CBackendResourceBinder::SetTextures(CRenderBackendFacade& backend, STextureList* T)
{
	if (m_T == T)
		return;
	m_T = T;

	u32 last_ps = 0;
	u32 last_vs = 0;

	if (!T)
		return;

	STextureList::iterator it = T->begin();
	STextureList::iterator end = T->end();
	for (; it != end; ++it)
	{
		std::pair<u32, ref_texture_legacy>& loader = *it;
		u32              load_id = loader.first;
		CTextureLegacy* load_surf = &*loader.second;

		if (load_id < 256) // pixel stage
		{
			if (load_id > last_ps)
				last_ps = load_id;
			if (m_texturesPS[load_id] != load_surf)
			{
				m_texturesPS[load_id] = load_surf;
#ifdef DEBUG
				backend.stat.textures++;
#endif
				if (load_surf)
					load_surf->bind(load_id);
				else
					backend.GetDevice()->SetTexture(load_id, nullptr);
			}
		}
		else // vertex stage
		{
			u32 load_id_remapped = load_id - 256;
			if (load_id_remapped > last_vs)
				last_vs = load_id_remapped;
			if (m_texturesVS[load_id_remapped] != load_surf)
			{
				m_texturesVS[load_id_remapped] = load_surf;
#ifdef DEBUG
				backend.stat.textures++;
#endif
				if (load_surf)
					load_surf->bind(load_id);
				else
					backend.GetDevice()->SetTexture(load_id, nullptr);
			}
		}
	}

	// clear remaining pixel stages
	for (++last_ps; last_ps < 16; ++last_ps)
	{
		if (m_texturesPS[last_ps] != nullptr)
		{
			m_texturesPS[last_ps] = nullptr;
			backend.GetDevice()->SetTexture(last_ps, nullptr);
		}
	}

	// clear remaining vertex stages
	for (++last_vs; last_vs < 5; ++last_vs)
	{
		if (m_texturesVS[last_vs] != nullptr)
		{
			m_texturesVS[last_vs] = nullptr;
			backend.GetDevice()->SetTexture(last_vs + 256, nullptr);
		}
	}
}

CTextureLegacy* CBackendResourceBinder::GetActiveTexture(u32 stage) const
{
	if (stage >= 256)
		return m_texturesVS[stage - 256];
	return m_texturesPS[stage];
}

void CBackendResourceBinder::SetPixelShaderLegacy(CRenderBackendFacade& backend, IDirect3DPixelShader9* ps, LPCSTR name)
{
	const bool cacheValid = (m_owner == EStateOwner::Legacy && m_psLegacy == ps);
	if (!cacheValid)
	{
		backend.stat.ps++;
		m_psLegacy = ps;
		backend.GetDevice()->SetPixelShader(ps);
		m_owner = EStateOwner::Legacy;
	}
#ifdef DEBUG
	m_psName = name;
#endif
}

void CBackendResourceBinder::SetVertexShaderLegacy(CRenderBackendFacade& backend, IDirect3DVertexShader9* vs, LPCSTR name)
{
	const bool cacheValid = (m_owner == EStateOwner::Legacy && m_vsLegacy == vs);
	if (!cacheValid)
	{
		backend.stat.vs++;
		m_vsLegacy = vs;
		backend.GetDevice()->SetVertexShader(vs);
		m_owner = EStateOwner::Legacy;
	}
#ifdef DEBUG
	m_vsName = name;
#endif
}

void CBackendResourceBinder::SetVertexDeclarationLegacy(CRenderBackendFacade& backend, IDirect3DVertexDeclaration9* decl)
{
	const bool cacheValid = (m_owner == EStateOwner::Legacy && m_declLegacy == decl);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.decl++;
#endif
		m_declLegacy = decl;
		backend.GetDevice()->SetVertexDeclaration(decl);
		m_owner = EStateOwner::Legacy;
	}
}

void CBackendResourceBinder::SetVertexBufferLegacy(CRenderBackendFacade& backend, IDirect3DVertexBuffer9* vb, u32 stride)
{
	const bool cacheValid = (m_owner == EStateOwner::Legacy &&
		m_vbLegacy == vb &&
		m_vbStrideLegacy == stride);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.vb++;
#endif
		m_vbLegacy = vb;
		m_vbStrideLegacy = stride;
		backend.GetDevice()->SetStreamSource(0, vb, 0, stride);
		m_owner = EStateOwner::Legacy;
	}
}

void CBackendResourceBinder::SetIndexBufferLegacy(CRenderBackendFacade& backend, IDirect3DIndexBuffer9* ib)
{
	const bool cacheValid = (m_owner == EStateOwner::Legacy && m_ibLegacy == ib);
	if (!cacheValid)
	{
#ifdef DEBUG
		backend.stat.ib++;
#endif
		m_ibLegacy = ib;
		backend.GetDevice()->SetIndices(ib);
		m_owner = EStateOwner::Legacy;
	}
}
////////////////////////////////////////////////////////////////////////////////
