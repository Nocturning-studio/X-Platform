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
CShaderProgram::~CShaderProgram() { Release(); }

CShaderProgram::CShaderProgram(CShaderProgram&& other) noexcept
		: m_type(other.m_type), 
		m_sourceFile(std::move(other.m_sourceFile)), 
		m_entry(std::move(other.m_entry)), 
		m_bytecodePtr(other.m_bytecodePtr), 
		m_bytecodeSize(other.m_bytecodeSize), 
		m_rhiHandle(other.m_rhiHandle), 
		m_rhi(other.m_rhi)
{
	other.m_bytecodePtr = nullptr;
	other.m_bytecodeSize = 0;
	other.m_rhiHandle = {};
	other.m_rhi = nullptr;
}

CShaderProgram& CShaderProgram::operator=(CShaderProgram&& other) noexcept
{
	if(this != &other)
	{
		Release();

		m_type = other.m_type;
		m_sourceFile = std::move(other.m_sourceFile);
		m_entry = std::move(other.m_entry);
		m_bytecodePtr = other.m_bytecodePtr;
		m_bytecodeSize = other.m_bytecodeSize;
		m_rhiHandle = other.m_rhiHandle;
		m_rhi = other.m_rhi;

		other.m_bytecodePtr = nullptr;
		other.m_bytecodeSize = 0;
		other.m_rhiHandle = {};
		other.m_rhi = nullptr;
	}
	return *this;
}

void CShaderProgram::Release()
{
	if(m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyShader(m_rhiHandle);

	m_rhiHandle = {};
	m_rhi = nullptr;
	m_bytecodePtr = nullptr;
	m_bytecodeSize = 0;
	m_sourceFile.clear();
	m_entry = "main";
	m_type = Type::Vertex;
}

HRESULT CShaderProgram::CompileFromFile(IRenderBackend& rhi, Type type, LPCSTR file, LPCSTR entry)
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

	const HRESULT hr = CompileFromMemory(rhi, type,
										 static_cast<LPCSTR>(reader->pointer()),
										 static_cast<UINT>(reader->length()),
										 localEntry.c_str(),
										 localFile.c_str(),
										 &includer);
	FS.r_close(reader);
	return hr;
}

HRESULT CShaderProgram::CompileFromMemory(IRenderBackend& rhi, Type type,
										  LPCSTR source, UINT size,
										  LPCSTR entry, LPCSTR debugName,
										  RHI_IncludeHandler* pInclude)
{
	Release();

	m_type = type;
	m_entry = (entry && entry[0]) ? entry : "main";
	if(debugName)
		m_sourceFile = debugName;

	RHI_ShaderCompileDesc desc;
	desc.source = source;
	desc.sourceSize = size;
	desc.sourceName = m_sourceFile.empty() ? nullptr : m_sourceFile.c_str();
	desc.entryPoint = m_entry.c_str();
	desc.type = (type == Type::Vertex) ? RHI_ShaderType::Vertex : RHI_ShaderType::Pixel;
	desc.includeHandler = pInclude;
	desc.debugName = m_sourceFile.empty() ? nullptr : m_sourceFile.c_str();
	desc.flags = RHI_ShaderCompile_Default;

	RHI_ShaderCompileResult result = rhi.CompileShader(desc);

	if(!result.IsValid())
	{
		Msg("! [ShaderProgram] Compile failed (%s, entry '%s'):\n%s",
			m_sourceFile.c_str(), m_entry.c_str(),
			result.errorMessage.empty() ? "unknown error" : result.errorMessage.c_str());
		return E_FAIL;
	}

	m_rhiHandle = result.shader;
	m_rhi = &rhi;

	// Кэшируем указатель на байткод для reflection. Указатель валиден,
	// пока RHI хранит shader (т.е. до DestroyShader в Release).
	const void* data = nullptr;
	size_t sz = 0;
	if(rhi.GetShaderBytecode(m_rhiHandle, &data, &sz))
	{
		m_bytecodePtr = data;
		m_bytecodeSize = sz;
	}

	return S_OK;
}

void CShaderProgram::Apply(IRenderBackend& rhi) const
{
	if(m_type == Type::Vertex)
		rhi.SetVertexShader(m_rhiHandle);
	else
		rhi.SetPixelShader(m_rhiHandle);
}
////////////////////////////////////////////////////////////////////////////////
