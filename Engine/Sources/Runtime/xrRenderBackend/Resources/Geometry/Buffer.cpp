////////////////////////////////////////////////////////////////////////////////
// Created: 01.10.2026 17:17:41
// Author: NS_Deathman
// File: Buffer.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Buffer.h"
////////////////////////////////////////////////////////////////////////////////
// ============================================================================
// CVertexBuffer
// ============================================================================

CVertexBuffer::~CVertexBuffer()
{
	DestroyRHI();
}

bool CVertexBuffer::Create(IRenderBackend& rhi, const RHI_BufferDesc& desc, const void* initialData)
{
	DestroyRHI();

	if (desc.sizeBytes == 0 || desc.stride == 0)
	{
		Msg("! [CVertexBuffer] invalid desc: size=%u, stride=%u", desc.sizeBytes, desc.stride);
		return false;
	}

	m_desc = desc;
	m_rhi = &rhi;

	m_rhiHandle = rhi.CreateVertexBuffer(desc, initialData);
	if (!m_rhiHandle.IsValid())
	{
		Msg("! [CVertexBuffer] CreateVertexBuffer failed (size=%u, stride=%u)",
			desc.sizeBytes, desc.stride);
		m_rhi = nullptr;
		return false;
	}
	return true;
}

void* CVertexBuffer::Lock(uint32_t offset, uint32_t size, uint32_t flags)
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return nullptr;
	return m_rhi->LockBuffer(m_rhiHandle, offset, size, flags);
}

void CVertexBuffer::Unlock()
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return;
	m_rhi->UnlockBuffer(m_rhiHandle);
}

void CVertexBuffer::Bind(IRenderBackend& rhi, uint32_t stream, uint32_t offset) const
{
	if (!m_rhiHandle.IsValid())
		return;
	rhi.SetVertexBuffer(stream, m_rhiHandle, offset, m_desc.stride);
}

void CVertexBuffer::DestroyRHI()
{
	if (m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyBuffer(m_rhiHandle);
	m_rhiHandle = {};
	m_rhi = nullptr;
}

// ============================================================================
// CIndexBuffer
// ============================================================================

CIndexBuffer::~CIndexBuffer()
{
	DestroyRHI();
}

bool CIndexBuffer::Create(IRenderBackend& rhi, const RHI_BufferDesc& desc, const void* initialData)
{
	DestroyRHI();

	if (desc.sizeBytes == 0)
	{
		Msg("! [CIndexBuffer] invalid desc: size=0");
		return false;
	}

	m_desc = desc;
	m_rhi = &rhi;

	m_rhiHandle = rhi.CreateIndexBuffer(desc, initialData);
	if (!m_rhiHandle.IsValid())
	{
		Msg("! [CIndexBuffer] CreateIndexBuffer failed (size=%u, fmt=%u)",
			desc.sizeBytes, (uint32_t)desc.indexFormat);
		m_rhi = nullptr;
		return false;
	}
	return true;
}

void* CIndexBuffer::Lock(uint32_t offset, uint32_t size, uint32_t flags)
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return nullptr;
	return m_rhi->LockBuffer(m_rhiHandle, offset, size, flags);
}

void CIndexBuffer::Unlock()
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return;
	m_rhi->UnlockBuffer(m_rhiHandle);
}

void CIndexBuffer::Bind(IRenderBackend& rhi) const
{
	if (!m_rhiHandle.IsValid())
		return;
	rhi.SetIndexBuffer(m_rhiHandle, m_desc.indexFormat);
}

void CIndexBuffer::DestroyRHI()
{
	if (m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyBuffer(m_rhiHandle);
	m_rhiHandle = {};
	m_rhi = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
