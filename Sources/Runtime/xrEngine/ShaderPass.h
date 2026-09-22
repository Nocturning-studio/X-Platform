////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "ShaderProgram.h"
////////////////////////////////////////////////////////////////////////////////
struct IDirect3DDevice9;
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CShaderPass
{
public:
	CShaderPass() = default;
	~CShaderPass() = default;

	CShaderPass(const CShaderPass&) = delete;
	CShaderPass& operator=(const CShaderPass&) = delete;

	// --- Конфигурация ---
	void SetVertexShader(LPCSTR file, LPCSTR entry = "main");
	void SetPixelShader(LPCSTR file, LPCSTR entry = "main");

	LPCSTR GetVertexShaderFile()  const { return m_vsFile.c_str(); }
	LPCSTR GetVertexShaderEntry() const { return m_vsEntry.c_str(); }
	LPCSTR GetPixelShaderFile()   const { return m_psFile.c_str(); }
	LPCSTR GetPixelShaderEntry()  const { return m_psEntry.c_str(); }

	// --- Компиляция ---
	BOOL Compile(IDirect3DDevice9* device);
	void Invalidate() { m_valid = false; }
	bool IsValid()    const { return m_valid; }

	// --- Рендеринг ---
	void Apply(IDirect3DDevice9* device) const;

	// --- Device lost / reset ---
	void OnDeviceLost();
	BOOL OnDeviceReset(IDirect3DDevice9* device);

	// Диагностика
	const CShaderProgram& GetVertexProgram() const { return m_vs; }
	const CShaderProgram& GetPixelProgram()  const { return m_ps; }

	IDirect3DVertexShader9* GetRawVertexShader() const { return static_cast<IDirect3DVertexShader9*>(m_vs.GetRawShader()); };
	IDirect3DPixelShader9* GetRawPixelShader() const { return static_cast<IDirect3DPixelShader9*>(m_ps.GetRawShader()); };

private:
	xr_string m_vsFile;
	xr_string m_vsEntry = "main";
	xr_string m_psFile;
	xr_string m_psEntry = "main";

	CShaderProgram m_vs;
	CShaderProgram m_ps;

	bool m_valid = false;
};
////////////////////////////////////////////////////////////////////////////////
