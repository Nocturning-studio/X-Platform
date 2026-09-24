////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderPass.h"
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

BOOL CShaderPass::Compile(IDirect3DDevice9* device)
{
	m_valid = false;

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

	// Данные буфера от старого шейдера больше не актуальны.
	m_constantsBuffer.Reset();

	m_valid = true;
	return TRUE;
}

void CShaderPass::Apply(IDirect3DDevice9* device) const
{
	m_vs.Apply(device);
	m_ps.Apply(device);
}

void CShaderPass::OnDeviceLost()
{
	m_vs.OnDeviceLost();
	m_ps.OnDeviceLost();
	m_valid = false;
}

BOOL CShaderPass::OnDeviceReset(IDirect3DDevice9* device)
{
	if (!m_vsFile.empty() && FAILED(m_vs.OnDeviceReset(device)))
		return FALSE;
	if (!m_psFile.empty() && FAILED(m_ps.OnDeviceReset(device)))
		return FALSE;

	m_valid = true;
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////
