////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include <string>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderProgram
{
public:
	enum class Type { Vertex, Pixel };

	CShaderProgram() = default;
	~CShaderProgram();

	CShaderProgram(const CShaderProgram&) = delete;
	CShaderProgram& operator=(const CShaderProgram&) = delete;
	CShaderProgram(CShaderProgram&& other) noexcept;
	CShaderProgram& operator=(CShaderProgram&& other) noexcept;

	HRESULT CompileFromFile(IRenderBackend& rhi,
							Type type,
							LPCSTR file,
							LPCSTR entry,
							const RHI_ShaderMacro* defines = nullptr);

	HRESULT CompileFromMemory(IRenderBackend& rhi,
							  Type type,
							  LPCSTR source,
							  UINT size,
							  LPCSTR entry,
							  LPCSTR debugName,
							  RHI_IncludeHandler* pInclude = nullptr,
							  const RHI_ShaderMacro* defines = nullptr);

	void Release();

	bool IsValid() const { return m_rhiHandle.IsValid(); }
	bool HasBytecode() const { return m_bytecodePtr != nullptr; }
	const void* GetBytecodePointer() const { return m_bytecodePtr; }
	UINT GetBytecodeSize() const { return static_cast<UINT>(m_bytecodeSize); }

	Type GetType() const { return m_type; }
	bool HasSourceFile() const { return !m_sourceFile.empty(); }
	LPCSTR GetSourceFile() const { return m_sourceFile.c_str(); }
	LPCSTR GetEntry() const { return m_entry.c_str(); }

	RHI_ShaderHandle GetRHIHandle() const { return m_rhiHandle; }

	void Apply(IRenderBackend& rhi) const;

private:
	Type m_type = Type::Vertex;
	std::string m_sourceFile;
	std::string m_entry = "main";

	const void* m_bytecodePtr = nullptr;
	size_t m_bytecodeSize = 0;

	RHI_ShaderHandle m_rhiHandle{};
	IRenderBackend* m_rhi = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
