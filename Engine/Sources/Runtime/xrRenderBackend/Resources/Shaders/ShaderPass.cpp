////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderPass.h"
#include <RenderBackend.h>
////////////////////////////////////////////////////////////////////////////////
void CShaderPass::SetVertexShader(LPCSTR file, LPCSTR entry)
{
	m_vsFile = file ? file : "";
	m_vsEntry = (entry && entry[0]) ? entry : "main";
	m_valid = false;
}

void CShaderPass::SetPixelShader(LPCSTR file, LPCSTR entry)
{
	m_psFile = file ? file : "";
	m_psEntry = (entry && entry[0]) ? entry : "main";
	m_valid = false;
}

BOOL CShaderPass::Compile(CRenderBackend& backend)
{
	m_valid = false;

	IDirect3DDevice9Ex* device = backend.GetDevice();
	if (!device)
	{
		Msg("! [ShaderPass] Compile: backend has no device");
		return FALSE;
	}

	// Vertex
	if (!m_vsFile.empty())
	{
		const HRESULT hr = m_vs.CompileFromFile(device, CShaderProgram::Type::Vertex, m_vsFile.c_str(), m_vsEntry.c_str());
		if (FAILED(hr))
			return FALSE;
	}
	else
	{
		m_vs.Release();
	}

	// Pixel
	if (!m_psFile.empty())
	{
		const HRESULT hr = m_ps.CompileFromFile(device, CShaderProgram::Type::Pixel, m_psFile.c_str(), m_psEntry.c_str());
		if (FAILED(hr))
			return FALSE;
	}
	else
	{
		m_ps.Release();
	}

	// Собираем таблицу констант из обоих стейджей.
	m_constants.Clear();

	CShaderConstantTable vsTable, psTable;
	vsTable.Parse(m_vs.GetBytecodePointer(), m_vs.GetBytecodeSize(), RC_dest_vertex_bit);
	psTable.Parse(m_ps.GetBytecodePointer(), m_ps.GetBytecodeSize(), RC_dest_pixel_bit);

	m_constants.Merge(vsTable);
	m_constants.Merge(psTable);

	m_samplers.clear();
	for (const auto& entry : m_constants.Samplers())
	{
		CShaderSamplerBinding b;
		b.name = entry.name;
		b.dx9Stage = entry.dx9Stage;
		b.type = entry.type;
		b.desc = CSamplerDesc::Default();
		m_samplers.push_back(std::move(b));
	}

	// Данные буфера от старого шейдера больше не актуальны.
	m_constantsBuffer.Reset();

	m_valid = true;
	return TRUE;
}

const CShaderSamplerBinding* CShaderPass::FindSampler(LPCSTR name) const
{
	if (!name) return nullptr;
	for (const auto& b : m_samplers)
		if (xr_strcmp(b.name.c_str(), name) == 0)
			return &b;
	return nullptr;
}

bool CShaderPass::SetTexture(LPCSTR samplerName, const ref_texture& tex)
{
	auto* b = const_cast<CShaderSamplerBinding*>(FindSampler(samplerName));
	if (!b) return false;
	b->texture = tex;
	return true;
}

bool CShaderPass::SetSamplerDesc(LPCSTR samplerName, const CSamplerDesc& desc)
{
	auto* b = const_cast<CShaderSamplerBinding*>(FindSampler(samplerName));
	if (!b) return false;
	b->desc = desc;
	return true;
}

void CShaderPass::ApplySamplers(CRenderBackend& backend) const
{
	IDirect3DDevice9Ex* device = backend.GetDevice();
	if (!device) return;

	for (const auto& b : m_samplers)
	{
		if (b.dx9Stage == uint32_t(-1))
			continue;

		CTexture* tex = b.texture._get();
		if (tex)
			tex->Bind(device, b.dx9Stage);
		else
			device->SetTexture(b.dx9Stage, nullptr);

		ApplySamplerDesc(device, b.dx9Stage, b.desc);
	}
}

void CShaderPass::Apply(CRenderBackend& backend)
{
	IDirect3DDevice9Ex* device = backend.GetDevice();
	if (!device) return;

	m_vs.Apply(device);
	m_ps.Apply(device);

	ApplySamplers(backend);

	m_constantsBuffer.Flush(device);
}

void CShaderPass::OnDeviceLost()
{
	m_vs.OnDeviceLost();
	m_ps.OnDeviceLost();
	m_valid = false;
}

BOOL CShaderPass::OnDeviceReset(CRenderBackend& backend)
{
	IDirect3DDevice9Ex* device = backend.GetDevice();
	if (!device) return FALSE;

	if (!m_vsFile.empty() && FAILED(m_vs.OnDeviceReset(device)))
		return FALSE;
	if (!m_psFile.empty() && FAILED(m_ps.OnDeviceReset(device)))
		return FALSE;

	m_valid = true;
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////
