#include "pch.h"
#include "xrBackendDX9.h"

RHI_BEGIN

ShaderHandle CRenderBackendDX9::AllocShaderHandle(DX9Shader* shader)
{
	u32 index;
	if (!m_FreeShaderIndices.empty())
	{
		index = m_FreeShaderIndices.top();
		m_FreeShaderIndices.pop();
		m_Shaders[index] = shader;
	}
	else
	{
		index = static_cast<u32>(m_Shaders.size());
		m_Shaders.push_back(shader);
	}
	return ShaderHandle{index};
}

DX9Shader* CRenderBackendDX9::GetShader(ShaderHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Shaders.size())
		return nullptr;
	return m_Shaders[handle.id];
}

void CRenderBackendDX9::FreeShaderHandle(ShaderHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Shaders.size())
		return;
	DX9Shader* shader = m_Shaders[handle.id];
	if (!shader)
	{
		Print("! [DX9] Double free of ShaderHandle(id=%u) detected.", handle.id);
		return;
	}
	shader->Release();
	delete shader;
	m_Shaders[handle.id] = nullptr;
	m_FreeShaderIndices.push(handle.id);
}

ConstantBufferHandle CRenderBackendDX9::AllocConstantBufferHandle(DX9ConstantBuffer* cb)
{
	u32 index;
	if (!m_FreeConstantBufferIndices.empty())
	{
		index = m_FreeConstantBufferIndices.top();
		m_FreeConstantBufferIndices.pop();
		m_ConstantBuffers[index] = cb;
	}
	else
	{
		index = static_cast<u32>(m_ConstantBuffers.size());
		m_ConstantBuffers.push_back(cb);
	}
	return ConstantBufferHandle{index};
}

DX9ConstantBuffer* CRenderBackendDX9::GetConstantBuffer(ConstantBufferHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_ConstantBuffers.size())
		return nullptr;
	return m_ConstantBuffers[handle.id];
}

void CRenderBackendDX9::FreeConstantBufferHandle(ConstantBufferHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_ConstantBuffers.size())
		return;
	DX9ConstantBuffer* cb = m_ConstantBuffers[handle.id];
	if (!cb)
	{
		Print("! [DX9] Double free of ConstantBufferHandle(id=%u) detected.", handle.id);
		return;
	}
	delete cb;
	m_ConstantBuffers[handle.id] = nullptr;
	m_FreeConstantBufferIndices.push(handle.id);
}

ShaderHandle CRenderBackendDX9::CreateShader(ShaderType type, const void* bytecode, size_t bytecodeSize)
{
	if (!m_pDevice)
		return ShaderHandle{};
	if (bytecode == nullptr || bytecodeSize == 0)
	{
		Print("! [DX9] CreateShader: invalid bytecode.");
		return ShaderHandle{};
	}

	DX9Shader* impl = new DX9Shader;
	impl->type = type;
	impl->bytecode.assign(static_cast<const u8*>(bytecode), static_cast<const u8*>(bytecode) + bytecodeSize);

	HRESULT hr = S_OK;
	if (type == ShaderType::Vertex)
	{
		hr = m_pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(impl->bytecode.data()), &impl->vs);
	}
	else if (type == ShaderType::Pixel)
	{
		hr = m_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(impl->bytecode.data()), &impl->ps);
	}
	else
	{
		Print("! [DX9] CreateShader: unsupported shader type %d", static_cast<int>(type));
		delete impl;
		return ShaderHandle{};
	}

	if (FAILED(hr))
	{
		Print("! [DX9] CreateShader failed (0x%08x) for type %d", hr, static_cast<int>(type));
		delete impl;
		return ShaderHandle{};
	}

	return AllocShaderHandle(impl);
}

void CRenderBackendDX9::DestroyShader(ShaderHandle handle)
{
	DX9Shader* shader = GetShader(handle);
	if (!shader)
	{
		Print("! [DX9] DestroyShader: invalid or already destroyed handle (id=%u).", handle.id);
		return;
	}
	FreeShaderHandle(handle);
}

void CRenderBackendDX9::SetShader(ShaderType type, ShaderHandle handle)
{
	if (!m_pDevice)
		return;

	DX9Shader* shader = nullptr;
	if (handle.IsValid())
	{
		shader = GetShader(handle);
		if (!shader)
		{
			Print("! [DX9] SetShader: invalid shader handle (id=%u).", handle.id);
			return;
		}
	}

	switch (type)
	{
	case ShaderType::Vertex:
		m_pDevice->SetVertexShader(shader ? shader->vs : nullptr);
		break;
	case ShaderType::Pixel:
		m_pDevice->SetPixelShader(shader ? shader->ps : nullptr);
		break;
	default:
		Print("! [DX9] SetShader: unsupported shader type %d", static_cast<int>(type));
		break;
	}
}

ConstantBufferHandle CRenderBackendDX9::CreateConstantBuffer(u32 size)
{
	if (size == 0 || size % 16 != 0)
	{
		Print("! [DX9] CreateConstantBuffer: size must be positive multiple of 16 (got %u)", size);
		return ConstantBufferHandle{};
	}

	DX9ConstantBuffer* impl = new DX9ConstantBuffer;
	impl->data.resize(size / sizeof(float)); // кратно 4 из-за проверки выше
	return AllocConstantBufferHandle(impl);
}

void CRenderBackendDX9::DestroyConstantBuffer(ConstantBufferHandle handle)
{
	DX9ConstantBuffer* cb = GetConstantBuffer(handle);
	if (!cb)
	{
		Print("! [DX9] DestroyConstantBuffer: invalid or already destroyed handle (id=%u).", handle.id);
		return;
	}
	FreeConstantBufferHandle(handle);
}

void CRenderBackendDX9::UpdateConstantBuffer(ConstantBufferHandle handle, u32 offset, const void* data, u32 size)
{
	DX9ConstantBuffer* cb = GetConstantBuffer(handle);
	if (!cb || !data)
	{
		Print("! [DX9] UpdateConstantBuffer: invalid handle or data pointer.");
		return;
	}

	if (offset + size > cb->data.size() * sizeof(float))
	{
		Print("! [DX9] UpdateConstantBuffer: offset+size (%u) exceeds buffer capacity (%zu bytes).", offset + size,
			  cb->data.size() * sizeof(float));
		return;
	}

	memcpy(cb->data.data() + offset / sizeof(float), data, size);
}

void CRenderBackendDX9::SetShaderConstantBuffer(ShaderType type, u32 startRegister, ConstantBufferHandle handle)
{
	if (!m_pDevice)
		return;

	DX9ConstantBuffer* cb = GetConstantBuffer(handle);
	if (!cb)
	{
		Print("! [DX9] SetShaderConstantBuffer: invalid buffer handle.");
		return;
	}

	switch (type)
	{
	case ShaderType::Vertex:
		m_pDevice->SetVertexShaderConstantF(startRegister, cb->data.data(),
											static_cast<UINT>(cb->data.size() / 4) // 4 float = 1 регистр
		);
		break;
	case ShaderType::Pixel:
		m_pDevice->SetPixelShaderConstantF(startRegister, cb->data.data(), static_cast<UINT>(cb->data.size() / 4));
		break;
	default:
		Print("! [DX9] SetShaderConstantBuffer: unsupported shader type %d", static_cast<int>(type));
		break;
	}
}

ShaderConstantLayout CRenderBackendDX9::ReflectConstantLayout(ShaderHandle handle)
{
	ShaderConstantLayout layout;
	DX9Shader* shader = GetShader(handle);
	if (!shader || shader->bytecode.empty())
		return layout;

	const void* pData = nullptr;
	UINT dwSize = 0;
	HRESULT hr = D3DXFindShaderComment(reinterpret_cast<const DWORD*>(shader->bytecode.data()),
									   MAKEFOURCC('C', 'T', 'A', 'B'), &pData, &dwSize);
	if (FAILED(hr) || !pData || dwSize < sizeof(D3DXSHADER_CONSTANTTABLE))
		return layout;

	const D3DXSHADER_CONSTANTTABLE* pTable = static_cast<const D3DXSHADER_CONSTANTTABLE*>(pData);
	const D3DXSHADER_CONSTANTINFO* info =
		reinterpret_cast<const D3DXSHADER_CONSTANTINFO*>(reinterpret_cast<const BYTE*>(pTable) + pTable->ConstantInfo);
	const BYTE* base = reinterpret_cast<const BYTE*>(pTable);

	u32 maxReg = 0;
	for (u32 i = 0; i < pTable->Constants; ++i, ++info)
	{
		LPCSTR name = LPCSTR(base + info->Name);
		ConstantType type = ConstantType::Float;
		ConstantClass cls = ConstantClass::Unknown;

		switch (info->RegisterSet)
		{
		case D3DXRS_BOOL:
			type = ConstantType::Bool;
			break;
		case D3DXRS_INT4:
			type = ConstantType::Int;
			break;
		case D3DXRS_FLOAT4:
			type = ConstantType::Float;
			break;
		case D3DXRS_SAMPLER:
			type = ConstantType::Sampler;
			break;
		default:
			continue;
		}

		const D3DXSHADER_TYPEINFO* T = (const D3DXSHADER_TYPEINFO*)(base + info->TypeInfo);
		switch (T->Class)
		{
		case D3DXPC_SCALAR:
			cls = ConstantClass::Scalar;
			break;
		case D3DXPC_VECTOR:
			cls = ConstantClass::Vector;
			break;
		case D3DXPC_MATRIX_ROWS:
			if (T->Columns == 4 && T->Rows == 4)
				cls = ConstantClass::MatrixRows_4x4;
			else if (T->Columns == 4 && T->Rows == 3)
				cls = (info->RegisterCount == 2) ? ConstantClass::MatrixRows_2x4 : ConstantClass::MatrixRows_3x4;
			break;
		case D3DXPC_MATRIX_COLUMNS:
			if (T->Columns == 4 && T->Rows == 4)
				cls = ConstantClass::MatrixColumns_4x4;
			else if (T->Columns == 4 && T->Rows == 3)
				cls = (info->RegisterCount == 2) ? ConstantClass::MatrixColumns_2x4 : ConstantClass::MatrixColumns_3x4;
			break;
		case D3DXPC_STRUCT:
			cls = ConstantClass::Struct;
			break;
		case D3DXPC_OBJECT:
			if (T->Type >= D3DXPT_SAMPLER && T->Type <= D3DXPT_SAMPLERCUBE)
				type = ConstantType::Sampler;
			else
				continue;
			break;
		default:
			continue;
		}

		if (type == ConstantType::Sampler)
		{
			// семплеры храним в layout с size=0
			layout.fields.push_back({name, static_cast<u32>(info->RegisterIndex * 16), 0u, type, ConstantClass::Object,
									 static_cast<u16>(info->RegisterIndex), static_cast<u16>(info->RegisterCount)});
			continue;
		}

		u32 offset = static_cast<u32>(info->RegisterIndex * 16);
		u32 size = static_cast<u32>(info->RegisterCount * 16);
		layout.fields.push_back({name, offset, size, type, cls, static_cast<u16>(info->RegisterIndex),
								 static_cast<u16>(info->RegisterCount)});

		u32 endReg = static_cast<u32>(info->RegisterIndex + info->RegisterCount);
		if (endReg > maxReg)
			maxReg = endReg;
	}
	layout.totalSize = maxReg * 16;
	layout.registerBase = 0; // DX9 всегда c0
	return layout;
}

RHI_END
