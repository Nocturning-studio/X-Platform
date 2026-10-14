////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
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

	IRenderBackend* rhi = backend.GetRHI();
	if(!rhi)
	{
		Msg("! [ShaderPass] Compile: backend has no RHI");
		return FALSE;
	}

	const RHI_ShaderMacro* macros = m_macros.Data();

	// Vertex
	if(!m_vsFile.empty())
	{
		const HRESULT hr = m_vs.CompileFromFile(*rhi, CShaderProgram::Type::Vertex, m_vsFile.c_str(), m_vsEntry.c_str(), macros);
		if(FAILED(hr))
			return FALSE;
	}
	else
	{
		m_vs.Release();
	}

	// Pixel
	if(!m_psFile.empty())
	{
		const HRESULT hr = m_ps.CompileFromFile(*rhi, CShaderProgram::Type::Pixel, m_psFile.c_str(), m_psEntry.c_str(), macros);
		if(FAILED(hr))
			return FALSE;
	}
	else
	{
		m_ps.Release();
	}

	// Reflection (D3D9-specific, но bytecode pointer теперь от RHI).
	m_constants.Clear();
	CShaderConstantTable vsTable, psTable;
	vsTable.Parse(m_vs.GetBytecodePointer(), m_vs.GetBytecodeSize(), RC_dest_vertex_bit);
	psTable.Parse(m_ps.GetBytecodePointer(), m_ps.GetBytecodeSize(), RC_dest_pixel_bit);
	m_constants.Merge(vsTable);
	m_constants.Merge(psTable);

	m_samplers.clear();
	for(const auto& entry : m_constants.Samplers())
	{
		CShaderSamplerBinding b;
		b.name = entry.name;
		b.dx9Stage = entry.dx9Stage;
		b.type = entry.type;
		b.desc = RHI_SamplerDesc::Default();
		m_samplers.push_back(std::move(b));
	}

	m_constantsBuffer.Reset();
	m_valid = true;
	return TRUE;
}

const CShaderSamplerBinding* CShaderPass::FindSampler(LPCSTR name) const
{
	if(!name)
		return nullptr;
	for(const auto& b : m_samplers)
		if(xr_strcmp(b.name.c_str(), name) == 0)
			return &b;
	return nullptr;
}

bool CShaderPass::SetTexture(LPCSTR samplerName, const ref_texture& tex)
{
	auto* b = const_cast<CShaderSamplerBinding*>(FindSampler(samplerName));
	if(!b)
		return false;
	b->texture = tex;
	return true;
}

bool CShaderPass::SetSamplerDesc(LPCSTR samplerName, const RHI_SamplerDesc& desc)
{
	auto* b = const_cast<CShaderSamplerBinding*>(FindSampler(samplerName));
	if(!b)
		return false;
	b->desc = desc;
	return true;
}

void CShaderPass::ApplySamplers(CRenderBackend& backend) const
{
	IRenderBackend* rhi = backend.GetRHI();
	if(!rhi)
		return;

	for(const auto& b : m_samplers)
	{
		if(b.dx9Stage == uint32_t(-1))
			continue;

		CTexture* tex = b.texture._get();
		if(tex)
			tex->Bind(*rhi, b.dx9Stage);
		else
			rhi->SetShaderResource(b.dx9Stage, RHI_TextureHandle{});

		rhi->SetSampler(b.dx9Stage, b.desc);
	}
}

void CShaderPass::Apply(CRenderBackend& backend)
{
	IRenderBackend* rhi = backend.GetRHI();
	if(!rhi)
		return;

	ApplySamplers(backend);
	m_constantsBuffer.Flush(*rhi);
}

void CShaderPass::Release()
{
	m_vs.Release();
	m_ps.Release();
	m_valid = false;
}
////////////////////////////////////////////////////////////////////////////////
