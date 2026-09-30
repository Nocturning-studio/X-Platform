////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "ShaderProgram.h"
#include "ShaderSampler.h"
#include "ShaderSamplerBinding.h"
#include "ShaderConstantTable.h"
#include "ShaderConstantBuffer.h"
////////////////////////////////////////////////////////////////////////////////
class CRenderBackend;
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderPass
{
public:
	CShaderPass() = default;
	~CShaderPass() = default;

	CShaderPass(const CShaderPass&) = delete;
	CShaderPass& operator=(const CShaderPass&) = delete;

	// --- Конфигурация ---
	void SetVertexShader(LPCSTR file, LPCSTR entry = "main");
	void SetPixelShader(LPCSTR file, LPCSTR entry = "main");

	bool SetConstant(LPCSTR name, float v) { return m_constantsBuffer.SetFloat(name, v, m_constants); }
	bool SetConstant(LPCSTR name, const fvec4& v) { return m_constantsBuffer.SetVector(name, v, m_constants); }
	bool SetConstant(LPCSTR name, const fmat4x4& m) { return m_constantsBuffer.SetMatrix(name, m, m_constants); }

	bool SetTexture(LPCSTR samplerName, const ref_texture& tex);
	bool SetSamplerDesc(LPCSTR samplerName, const CSamplerDesc& desc);

	const xr_vector<CShaderSamplerBinding>& Samplers() const { return m_samplers; }
	const CShaderSamplerBinding* FindSampler(LPCSTR name) const;
	void ApplySamplers(CRenderBackend& backend) const;

	void FlushConstants(CRenderBackend& backend) const;
	void ResetConstants() { m_constantsBuffer.Reset(); }

	const CShaderConstantTable& Constants() const { return m_constants; }
	CShaderConstantBuffer& ConstantBuffer() { return m_constantsBuffer; }

	LPCSTR GetVertexShaderFile()  const { return m_vsFile.c_str(); }
	LPCSTR GetVertexShaderEntry() const { return m_vsEntry.c_str(); }
	LPCSTR GetPixelShaderFile()   const { return m_psFile.c_str(); }
	LPCSTR GetPixelShaderEntry()  const { return m_psEntry.c_str(); }

	// --- Компиляция ---
	BOOL Compile(CRenderBackend& backend);
	void Invalidate() { m_valid = false; }
	bool IsValid() const { return m_valid; }
	void Release();

	// --- Рендеринг ---
	void Apply(CRenderBackend& backend);

	// --- Device reset ---
	BOOL OnDeviceReset(CRenderBackend& backend);

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

	CShaderConstantTable m_constants;
	CShaderConstantBuffer m_constantsBuffer;

	xr_vector<CShaderSamplerBinding> m_samplers;

	bool m_valid = false;
};
////////////////////////////////////////////////////////////////////////////////
