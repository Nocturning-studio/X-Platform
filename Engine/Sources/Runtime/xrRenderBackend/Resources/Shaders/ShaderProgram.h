////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include <d3dcommon.h>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderProgram
{
  public:
	enum class Type
	{
		Vertex,
		Pixel
	};

	CShaderProgram() = default;
	~CShaderProgram();

	CShaderProgram(const CShaderProgram&) = delete;
	CShaderProgram& operator=(const CShaderProgram&) = delete;
	CShaderProgram(CShaderProgram&& other) noexcept;
	CShaderProgram& operator=(CShaderProgram&& other) noexcept;

	HRESULT CompileFromFile(IRenderBackend& rhi,
							IDirect3DDevice9* device,
							Type type,
							LPCSTR file,
							LPCSTR entry);

	HRESULT CompileFromMemory(IRenderBackend& rhi,
							  IDirect3DDevice9* device,
							  Type type,
							  LPCSTR source,
							  UINT size,
							  LPCSTR entry,
							  LPCSTR debugName,
							  ID3DInclude* pInclude = nullptr);

	void Release();

	bool IsValid() const { return m_rhiHandle.IsValid(); }
	bool HasBytecode() const;
	const void* GetBytecodePointer() const;
	UINT GetBytecodeSize() const;

	Type GetType() const { return m_type; }
	bool HasSourceFile() const { return !m_sourceFile.empty(); }
	LPCSTR GetSourceFile() const { return m_sourceFile.c_str(); }
	LPCSTR GetEntry() const { return m_entry.c_str(); }

	RHI_ShaderHandle GetRHIHandle() const { return m_rhiHandle; }

	void Apply(IRenderBackend& rhi) const;

  private:
	HRESULT CreateShaderObject(IRenderBackend& rhi);

	Type m_type = Type::Vertex;
	std::string m_sourceFile;
	std::string m_entry = "main";
	ID3DBlob* m_bytecode = nullptr;
	RHI_ShaderHandle m_rhiHandle{};
	IRenderBackend* m_rhi = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
