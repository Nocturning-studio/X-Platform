////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderProgram.h"
#include "ShaderIncluder.h"
#include <xrEngine/defines.h>
////////////////////////////////////////////////////////////////////////////////
namespace
{
void BuildShaderTarget(char (&out)[16], CShaderProgram::Type type, IDirect3DDevice9* device)
{
	D3DCAPS9 caps{};
	device->GetDeviceCaps(&caps);

	const DWORD version = (type == CShaderProgram::Type::Vertex)
							  ? caps.VertexShaderVersion
							  : caps.PixelShaderVersion;

	const UINT major = D3DSHADER_VERSION_MAJOR(version);
	const UINT minor = D3DSHADER_VERSION_MINOR(version);
	const char* prefix = (type == CShaderProgram::Type::Vertex) ? "vs" : "ps";

	sprintf_s(out, "%s_%u_%u", prefix, major, minor);
}

HRESULT CompileSourceToBytecode(LPCSTR source,
								UINT size,
								LPCSTR sourceName,
								LPCSTR entry,
								LPCSTR target,
								ID3DBlob** outBytecode,
								ID3DBlob** outErrors,
								ID3DInclude* pInclude)
{
	UINT flags1 = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	const UINT flags2 = 0;

	return D3DCompile(source, size, sourceName, nullptr, pInclude,
					  entry, target, flags1, flags2,
					  outBytecode, outErrors);
}
} // namespace

////////////////////////////////////////////////////////////////////////////////

CShaderProgram::~CShaderProgram()
{
	Release();
}

CShaderProgram::CShaderProgram(CShaderProgram&& other) noexcept
	: m_type(other.m_type)
	, m_sourceFile(std::move(other.m_sourceFile))
	, m_entry(std::move(other.m_entry))
	, m_bytecode(other.m_bytecode)
	, m_rhiHandle(other.m_rhiHandle)
	, m_rhi(other.m_rhi)
{
	other.m_bytecode = nullptr;
	other.m_rhiHandle = {};
	other.m_rhi = nullptr;
}

CShaderProgram& CShaderProgram::operator=(CShaderProgram&& other) noexcept
{
	if (this != &other)
	{
		Release();

		m_type       = other.m_type;
		m_sourceFile = std::move(other.m_sourceFile);
		m_entry      = std::move(other.m_entry);
		m_bytecode   = other.m_bytecode;
		m_rhiHandle  = other.m_rhiHandle;
		m_rhi        = other.m_rhi;

		other.m_bytecode = nullptr;
		other.m_rhiHandle = {};
		other.m_rhi = nullptr;
	}
	return *this;
}

bool CShaderProgram::HasBytecode() const 
{ 
	return m_bytecode != nullptr; 
}

const void* CShaderProgram::GetBytecodePointer() const 
{ 
	return m_bytecode ? m_bytecode->GetBufferPointer() : nullptr; 
}

UINT CShaderProgram::GetBytecodeSize() const 
{ 
	return m_bytecode ? static_cast<UINT>(m_bytecode->GetBufferSize()) : 0; 
}

void CShaderProgram::Release()
{
	if (m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyShader(m_rhiHandle);

	m_rhiHandle = {};
	m_rhi       = nullptr;

	_RELEASE(m_bytecode);
	m_sourceFile.clear();
	m_entry = "main";
	m_type  = Type::Vertex;
}

HRESULT CShaderProgram::CreateShaderObject(IRenderBackend& rhi)
{
	if (!m_bytecode)
		return E_FAIL;

	const void*  bytecode = m_bytecode->GetBufferPointer();
	const size_t size     = m_bytecode->GetBufferSize();
	const char*  name     = m_sourceFile.empty() ? nullptr : m_sourceFile.c_str();

	const RHI_ShaderHandle h = (m_type == Type::Vertex)
		? rhi.CreateVertexShader(bytecode, size, name)
		: rhi.CreatePixelShader(bytecode, size, name);

	if (!h.IsValid())
	{
		Msg("! [ShaderProgram] CreateShaderObject failed for '%s'", m_sourceFile.c_str());
		return E_FAIL;
	}

	m_rhiHandle = h;
	m_rhi       = &rhi;
	return S_OK;
}

HRESULT CShaderProgram::CompileFromFile(IRenderBackend& rhi,
										IDirect3DDevice9* device,
										Type type,
										LPCSTR file,
										LPCSTR entry)
{
	const xr_string localFile  = file ? file : "";
	const xr_string localEntry = (entry && entry[0]) ? entry : "main";

	string_path fullPath;
	strcpy_s(fullPath, localFile.c_str());
	FS.update_path(fullPath, "$engine_shaders$", fullPath);

	IReader* reader = FS.r_open(fullPath);
	if (!reader)
	{
		Msg("! [ShaderProgram] Cannot open shader file: %s", fullPath);
		return E_FAIL;
	}

	CShaderIncluder includer;

	const HRESULT hr = CompileFromMemory(rhi, device, type,
										 static_cast<LPCSTR>(reader->pointer()),
										 static_cast<UINT>(reader->length()),
										 localEntry.c_str(),
										 localFile.c_str(),
										 &includer);
	FS.r_close(reader);
	return hr;
}

HRESULT CShaderProgram::CompileFromMemory(IRenderBackend& rhi,
										  IDirect3DDevice9* device,
										  Type type,
										  LPCSTR source,
										  UINT size,
										  LPCSTR entry,
										  LPCSTR debugName,
										  ID3DInclude* pInclude)
{
	Release();

	m_type  = type;
	m_entry = (entry && entry[0]) ? entry : "main";
	if (debugName)
		m_sourceFile = debugName;

	char target[16];
	BuildShaderTarget(target, type, device);

	ID3DBlob* errors = nullptr;
	HRESULT hr = CompileSourceToBytecode(source, size,
										 m_sourceFile.c_str(),
										 m_entry.c_str(), target,
										 &m_bytecode, &errors,
										 pInclude);

	if (FAILED(hr))
	{
		const char* msg = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "unknown error";
		Msg("! [ShaderProgram] Compile failed (%s, entry '%s'):\n%s", m_sourceFile.c_str(), m_entry.c_str(), msg);
		_RELEASE(errors);
		return hr;
	}

	_RELEASE(errors);
	return CreateShaderObject(rhi);
}

void CShaderProgram::Apply(IRenderBackend& rhi) const
{
	if (m_type == Type::Vertex)
		rhi.SetVertexShader(m_rhiHandle);
	else
		rhi.SetPixelShader(m_rhiHandle);
}
////////////////////////////////////////////////////////////////////////////////
