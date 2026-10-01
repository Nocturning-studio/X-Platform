////////////////////////////////////////////////////////////////////////////////
// Created: 01.10.2026 19:49:20
// Author: NS_Deathman
// File: xrBackendDX_Shader.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
// ============================================================================
// Pool management
// ============================================================================

RHI_ShaderHandle CRenderBackendDX9::AllocShaderHandle(DX9Shader* sh)
{
	uint32_t idx;
	if (!m_freeShaderIndices.empty())
	{
		idx = m_freeShaderIndices.top();
		m_freeShaderIndices.pop();
		m_shaders[idx] = sh;
	}
	else
	{
		idx = static_cast<uint32_t>(m_shaders.size());
		m_shaders.push_back(sh);
	}
	return RHI_ShaderHandle{ idx };
}

DX9Shader* CRenderBackendDX9::GetShader(RHI_ShaderHandle h) const
{
	if (!h.IsValid() || h.id >= m_shaders.size())
		return nullptr;
	return m_shaders[h.id];
}

void CRenderBackendDX9::FreeShaderHandle(RHI_ShaderHandle h)
{
	if (!h.IsValid() || h.id >= m_shaders.size())
		return;

	DX9Shader* sh = m_shaders[h.id];
	if (!sh)
	{
		Msg("! [DX9] Double free of RHI_ShaderHandle(id=%u) detected, ignoring.", h.id);
		return;
	}

	// Разбиндить, если привязан. Иначе D3D9 получит висячий указатель.
	if (sh->isVertex && m_currentVS == sh->vs)
	{
		if (m_pDevice)
			m_pDevice->SetVertexShader(nullptr);
		m_currentVS = nullptr;
	}
	else if (!sh->isVertex && m_currentPS == sh->ps)
	{
		if (m_pDevice)
			m_pDevice->SetPixelShader(nullptr);
		m_currentPS = nullptr;
	}

	if (sh->vs) sh->vs->Release();
	if (sh->ps) sh->ps->Release();

	delete sh;
	m_shaders[h.id] = nullptr;
	m_freeShaderIndices.push(h.id);
}

// ============================================================================
// Create
// ============================================================================

RHI_ShaderHandle CRenderBackendDX9::CreateVertexShader(const void* bytecode, size_t size, const char* debugName)
{
	if (!m_pDevice || !bytecode || size == 0)
	{
		Msg("! [DX9] CreateVertexShader: invalid args (device=%p, bytecode=%p, size=%zu)",
			m_pDevice, bytecode, size);
		return {};
	}

	const DWORD* code = static_cast<const DWORD*>(bytecode);

	IDirect3DVertexShader9* vs = nullptr;
	const HRESULT hr = m_pDevice->CreateVertexShader(code, &vs);
	if (FAILED(hr) || !vs)
	{
		Msg("! [DX9] CreateVertexShader failed (hr=0x%08x, size=%zu)", hr, size);
		return {};
	}

	DX9Shader* sh = new DX9Shader;
	sh->vs = vs;
	sh->isVertex = true;
	return AllocShaderHandle(sh);
}

RHI_ShaderHandle CRenderBackendDX9::CreatePixelShader(const void* bytecode, size_t size, const char* debugName)
{
	if (!m_pDevice || !bytecode || size == 0)
	{
		Msg("! [DX9] CreatePixelShader: invalid args (device=%p, bytecode=%p, size=%zu)",
			m_pDevice, bytecode, size);
		return {};
	}

	const DWORD* code = static_cast<const DWORD*>(bytecode);

	IDirect3DPixelShader9* ps = nullptr;
	const HRESULT hr = m_pDevice->CreatePixelShader(code, &ps);
	if (FAILED(hr) || !ps)
	{
		Msg("! [DX9] CreatePixelShader failed (hr=0x%08x, size=%zu)", hr, size);
		return {};
	}

	DX9Shader* sh = new DX9Shader;
	sh->ps = ps;
	sh->isVertex = false;
	return AllocShaderHandle(sh);
}

void CRenderBackendDX9::DestroyShader(RHI_ShaderHandle handle)
{
	FreeShaderHandle(handle);
}

// ============================================================================
// Bind
// ============================================================================

void CRenderBackendDX9::SetVertexShader(RHI_ShaderHandle handle)
{
	if (!m_pDevice)
		return;

	IDirect3DVertexShader9* native = nullptr;
	if (handle.IsValid())
	{
		DX9Shader* sh = GetShader(handle);
		if (!sh || !sh->isVertex)
		{
			Msg("! [DX9] SetVertexShader: invalid handle (id=%u)", handle.id);
			return;
		}
		native = sh->vs;
	}

	if (m_currentVS == native)
		return;

	const HRESULT hr = m_pDevice->SetVertexShader(native);
	if (FAILED(hr))
	{
		Msg("! [DX9] SetVertexShader failed (hr=0x%08x)", hr);
		return;
	}

	m_currentVS = native;
}

void CRenderBackendDX9::SetPixelShader(RHI_ShaderHandle handle)
{
	if (!m_pDevice)
		return;

	IDirect3DPixelShader9* native = nullptr;
	if (handle.IsValid())
	{
		DX9Shader* sh = GetShader(handle);
		if (!sh || sh->isVertex)
		{
			Msg("! [DX9] SetPixelShader: invalid handle (id=%u)", handle.id);
			return;
		}
		native = sh->ps;
	}

	if (m_currentPS == native)
		return;

	const HRESULT hr = m_pDevice->SetPixelShader(native);
	if (FAILED(hr))
	{
		Msg("! [DX9] SetPixelShader failed (hr=0x%08x)", hr);
		return;
	}

	m_currentPS = native;
}
////////////////////////////////////////////////////////////////////////////////
