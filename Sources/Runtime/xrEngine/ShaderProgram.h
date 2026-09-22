////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
struct ID3DXBuffer;
struct ID3DXInclude;
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CShaderProgram
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

	// Загрузить файл из "$engine_shaders$" и скомпилировать.
	HRESULT CompileFromFile(IDirect3DDevice9* device, 
							Type type,
							LPCSTR file, 
							LPCSTR entry);

	// Скомпилировать из памяти
	HRESULT CompileFromMemory(IDirect3DDevice9* device, 
							  Type type,
							  LPCSTR source, 
							  UINT size,
							  LPCSTR entry, 
							  LPCSTR debugName,
							  ID3DXInclude* pInclude = nullptr);

	// Полный сброс (байткод + D3D-объект).
	void Release();

	// Device lost / reset. Байткод сохраняется, D3D-объект пересоздаётся.
	void OnDeviceLost();
	HRESULT OnDeviceReset(IDirect3DDevice9* device);

	bool IsValid() const { return m_shader != nullptr; }
	bool HasBytecode() const { return m_bytecode != nullptr; }

	Type GetType() const { return m_type; }
	bool HasSourceFile() const { return !m_sourceFile.empty(); }
	LPCSTR GetSourceFile() const { return m_sourceFile.c_str(); }
	LPCSTR GetEntry() const { return m_entry.c_str(); }
	void* GetRawShader() const { return m_shader; }

	// Привязка стейджа к устройству.
	void Apply(IDirect3DDevice9* device) const;

  private:
	HRESULT CreateShaderObject(IDirect3DDevice9* device);

	Type m_type = Type::Vertex;
	std::string m_sourceFile;
	std::string m_entry = "main";
	ID3DXBuffer* m_bytecode = nullptr;
	void* m_shader = nullptr; // IDirect3DVertexShader9* | IDirect3DPixelShader9*
};
////////////////////////////////////////////////////////////////////////////////
