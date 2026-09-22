////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderProgram.h"
#include "ShaderIncluder.h"
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

	return D3DCompile(source, size,
					  sourceName, // вставляет #line в байткод
					  nullptr,	  // defines
					  pInclude,
					  entry, target,
					  flags1, flags2,
					  outBytecode, outErrors);
}

} // namespace

CShaderProgram::~CShaderProgram()
{
	Release();
}

CShaderProgram::CShaderProgram(CShaderProgram&& other) noexcept
	: m_type(other.m_type), m_sourceFile(std::move(other.m_sourceFile)), m_entry(std::move(other.m_entry)), m_bytecode(other.m_bytecode), m_shader(other.m_shader)
{
	other.m_bytecode = nullptr;
	other.m_shader = nullptr;
}

CShaderProgram& CShaderProgram::operator=(CShaderProgram&& other) noexcept
{
	if(this != &other)
	{
		Release();
		m_type = other.m_type;
		m_sourceFile = std::move(other.m_sourceFile);
		m_entry = std::move(other.m_entry);
		m_bytecode = other.m_bytecode;
		m_shader = other.m_shader;
		other.m_bytecode = nullptr;
		other.m_shader = nullptr;
	}
	return *this;
}

void CShaderProgram::Release()
{
	if(m_shader)
	{
		if(m_type == Type::Vertex)
			static_cast<IDirect3DVertexShader9*>(m_shader)->Release();
		else
			static_cast<IDirect3DPixelShader9*>(m_shader)->Release();
		m_shader = nullptr;
	}

	_RELEASE(m_bytecode);
	m_sourceFile.clear();
	m_entry = "main";
}

HRESULT CShaderProgram::CreateShaderObject(IDirect3DDevice9* device)
{
	if(!m_bytecode)
		return E_FAIL;

	const DWORD* code = static_cast<const DWORD*>(m_bytecode->GetBufferPointer());
	HRESULT hr = E_FAIL;

	if(m_type == Type::Vertex)
	{
		IDirect3DVertexShader9* vs = nullptr;
		hr = device->CreateVertexShader(code, &vs);
		m_shader = vs;
	}
	else
	{
		IDirect3DPixelShader9* ps = nullptr;
		hr = device->CreatePixelShader(code, &ps);
		m_shader = ps;
	}

	if(FAILED(hr))
	{
		Msg("! [ShaderProgram] CreateShaderObject failed for '%s' (hr=0x%08X)",
			m_sourceFile.c_str(), hr);
	}
	return hr;
}

HRESULT CShaderProgram::CompileFromFile(IDirect3DDevice9* device,
										Type type,
										LPCSTR file,
										LPCSTR entry)
{
	const xr_string localFile = file ? file : "";
	const xr_string localEntry = (entry && entry[0]) ? entry : "main";

	string_path fullPath;
	strcpy_s(fullPath, localFile.c_str());
	FS.update_path(fullPath, "$engine_shaders$", fullPath);

	IReader* reader = FS.r_open(fullPath);
	if(!reader)
	{
		Msg("! [ShaderProgram] Cannot open shader file: %s", fullPath);
		return E_FAIL;
	}

	CShaderIncluder includer;

	const HRESULT hr = CompileFromMemory(device,
										 type,
										 static_cast<LPCSTR>(reader->pointer()),
										 static_cast<UINT>(reader->length()),
										 localEntry.c_str(),
										 localFile.c_str(),
										 &includer);
	FS.r_close(reader);
	return hr;
}

HRESULT CShaderProgram::CompileFromMemory(IDirect3DDevice9* device,
										  Type type,
										  LPCSTR source,
										  UINT size,
										  LPCSTR entry,
										  LPCSTR debugName,
										  ID3DInclude* pInclude)
{
	Release();
	m_type = type;
	m_entry = (entry && entry[0]) ? entry : "main";
	if(debugName)
		m_sourceFile = debugName;

	char target[16];
	BuildShaderTarget(target, type, device);

	ID3DBlob* errors = nullptr;
	HRESULT hr = CompileSourceToBytecode(source, size,
										 m_sourceFile.c_str(), // pSourceName
										 m_entry.c_str(), target,
										 &m_bytecode, &errors,
										 pInclude);

	if(FAILED(hr))
	{
		const char* msg = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "unknown error";
		Msg("! [ShaderProgram] Compile failed (%s, entry '%s'):\n%s", m_sourceFile.c_str(), m_entry.c_str(), msg);
		_RELEASE(errors);
		return hr;
	}

	_RELEASE(errors);
	return CreateShaderObject(device);
}

void CShaderProgram::OnDeviceLost()
{
	if(m_shader)
	{
		if(m_type == Type::Vertex)
			static_cast<IDirect3DVertexShader9*>(m_shader)->Release();
		else
			static_cast<IDirect3DPixelShader9*>(m_shader)->Release();
		m_shader = nullptr;
	}
}

HRESULT CShaderProgram::OnDeviceReset(IDirect3DDevice9* device)
{
	if(m_shader)
		return S_OK;
	if(!m_bytecode)
		return E_FAIL;
	return CreateShaderObject(device);
}

void CShaderProgram::Apply(IDirect3DDevice9* device) const
{
	if(m_type == Type::Vertex)
		device->SetVertexShader(static_cast<IDirect3DVertexShader9*>(m_shader));
	else
		device->SetPixelShader(static_cast<IDirect3DPixelShader9*>(m_shader));
}
////////////////////////////////////////////////////////////////////////////////
