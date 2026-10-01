////////////////////////////////////////////////////////////////////////////////
// Created: 01.10.2026 17:20:27
// Author: NS_Deathman
// File: VertexDeclaration.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////
CVertexDeclaration::~CVertexDeclaration()
{
	DestroyRHI();
}

bool CVertexDeclaration::Create(IRenderBackend& rhi, const RHI_InputLayoutDesc& desc)
{
	DestroyRHI();

	if (desc.elements.empty())
	{
		Msg("! [CVertexDeclaration] empty layout");
		return false;
	}

	m_desc = desc;
	m_rhi = &rhi;

	m_rhiHandle = rhi.CreateInputLayout(desc);
	if (!m_rhiHandle.IsValid())
	{
		Msg("! [CVertexDeclaration] CreateInputLayout failed (elems=%u)",
			(uint32_t)desc.elements.size());
		m_rhi = nullptr;
		return false;
	}
	return true;
}

void CVertexDeclaration::Bind(IRenderBackend& rhi) const
{
	if (!m_rhiHandle.IsValid())
		return;
	rhi.SetInputLayout(m_rhiHandle);
}

void CVertexDeclaration::DestroyRHI()
{
	if (m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyInputLayout(m_rhiHandle);
	m_rhiHandle = {};
	m_rhi = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
