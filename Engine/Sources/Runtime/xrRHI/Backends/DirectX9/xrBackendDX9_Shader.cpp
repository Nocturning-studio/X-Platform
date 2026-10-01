////////////////////////////////////////////////////////////////////////////////
// Created: 01.10.2026 19:49:20
// Author: NS_Deathman
// File: xrBackendDX_Shader.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{

// ============================================================================
// Адаптер RHI_IncludeHandler -> ID3DInclude
// ============================================================================
class DX9IncludeAdapter : public ID3DInclude
{
  public:
	explicit DX9IncludeAdapter(RHI_IncludeHandler* h) : m_handler(h) {}

	HRESULT __stdcall Open(D3D_INCLUDE_TYPE type, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
	{
		if (!m_handler || !pName || !ppData || !pBytes)
			return E_FAIL;

		const RHI_IncludeType rhiType = (type == D3D_INCLUDE_LOCAL)
			? RHI_IncludeType::Local
			: RHI_IncludeType::System;

		const void* data = nullptr;
		size_t size = 0;
		if (!m_handler->Open(rhiType, pName, pParentData, &data, &size))
			return E_FAIL;

		*ppData = data;
		*pBytes = static_cast<UINT>(size);
		return S_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData) override
	{
		if (m_handler) m_handler->Close(pData);
		return S_OK;
	}

  private:
	RHI_IncludeHandler* m_handler;
};

// ============================================================================
// Построение профиля: vs_3_0 / ps_3_0 из caps устройства
// ============================================================================
void BuildShaderTarget(char (&out)[16], RHI_ShaderType type, IDirect3DDevice9* device)
{
	D3DCAPS9 caps{};
	device->GetDeviceCaps(&caps);

	const DWORD version = (type == RHI_ShaderType::Vertex)
		? caps.VertexShaderVersion
		: caps.PixelShaderVersion;

	const UINT major = D3DSHADER_VERSION_MAJOR(version);
	const UINT minor = D3DSHADER_VERSION_MINOR(version);
	const char* prefix = (type == RHI_ShaderType::Vertex) ? "vs" : "ps";

	sprintf_s(out, "%s_%u_%u", prefix, major, minor);
}

} // namespace

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
	if (sh->blob) sh->blob->Release();

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

// ============================================================================
// CompileShader
// ============================================================================

RHI_ShaderCompileResult CRenderBackendDX9::CompileShader(const RHI_ShaderCompileDesc& desc)
{
	RHI_ShaderCompileResult result;

	if (!m_pDevice)
	{
		result.errorMessage = "device is null";
		return result;
	}
	if (!desc.source || desc.sourceSize == 0)
	{
		result.errorMessage = "empty shader source";
		return result;
	}

	char target[16];
	BuildShaderTarget(target, desc.type, m_pDevice);

	DX9IncludeAdapter includeAdapter(desc.includeHandler);

	// Маппинг RHI-флагов в D3DCOMPILE_*.
	UINT flags1 = 0;
	if (desc.flags & RHI_ShaderCompile_Debug)             flags1 |= D3DCOMPILE_DEBUG;
	if (desc.flags & RHI_ShaderCompile_SkipOptimize)      flags1 |= D3DCOMPILE_SKIP_OPTIMIZATION;
	if (desc.flags & RHI_ShaderCompile_RowMajor)          flags1 |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	if (desc.flags & RHI_ShaderCompile_ColumnMajor)       flags1 |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
	if (desc.flags & RHI_ShaderCompile_PreferFlowControl) flags1 |= D3DCOMPILE_PREFER_FLOW_CONTROL;
	if (desc.flags & RHI_ShaderCompile_OptimizeLevel3)    flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

	ID3DBlob* bytecode = nullptr;
	ID3DBlob* errors = nullptr;

	const HRESULT hr = D3DCompile(desc.source, desc.sourceSize,
								  desc.sourceName ? desc.sourceName : "<memory>",
								  nullptr,                                            // defines (TODO: маппинг RHI_ShaderMacro)
								  desc.includeHandler ? &includeAdapter : nullptr,
								  desc.entryPoint ? desc.entryPoint : "main",
								  target, flags1, 0,
								  &bytecode, &errors);

	if (FAILED(hr))
	{
		if (errors)
		{
			result.errorMessage.assign(
				static_cast<const char*>(errors->GetBufferPointer()),
				errors->GetBufferSize());
			errors->Release();
		}
		else
		{
			result.errorMessage = "unknown compile error";
		}
		return result;
	}
	if (errors) errors->Release();

	// Создаём shader-объект. Bytecode остаётся у нас до DestroyShader - нужен для reflection.
	DX9Shader* sh = new DX9Shader;
	sh->isVertex = (desc.type == RHI_ShaderType::Vertex);
	sh->blob = bytecode;

	const DWORD* code = static_cast<const DWORD*>(bytecode->GetBufferPointer());

	HRESULT hrCreate = E_FAIL;
	if (sh->isVertex)
	{
		IDirect3DVertexShader9* vs = nullptr;
		hrCreate = m_pDevice->CreateVertexShader(code, &vs);
		sh->vs = vs;
	}
	else
	{
		IDirect3DPixelShader9* ps = nullptr;
		hrCreate = m_pDevice->CreatePixelShader(code, &ps);
		sh->ps = ps;
	}

	if (FAILED(hrCreate))
	{
		Msg("! [DX9] CompileShader: Create*Shader failed (hr=0x%08x)", hrCreate);
		bytecode->Release();
		delete sh;
		result.errorMessage = "Create*Shader failed";
		return result;
	}

	result.shader = AllocShaderHandle(sh);
	return result;
}

// ============================================================================
// GetShaderBytecode
// ============================================================================

bool CRenderBackendDX9::GetShaderBytecode(RHI_ShaderHandle handle,
	const void** outData, size_t* outSize)
{
	if (!outData || !outSize)
		return false;
	*outData = nullptr;
	*outSize = 0;

	DX9Shader* sh = GetShader(handle);
	if (!sh || !sh->blob)
		return false;

	*outData = sh->blob->GetBufferPointer();
	*outSize = sh->blob->GetBufferSize();
	return true;
}
////////////////////////////////////////////////////////////////////////////////
