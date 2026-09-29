////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Geometry.h"
////////////////////////////////////////////////////////////////////////////////

namespace
{
D3DPRIMITIVETYPE ToD3DPrimitive(EPrimitiveTopology t)
{
	switch(t)
	{
	case EPrimitiveTopology::PointList:
		return D3DPT_POINTLIST;
	case EPrimitiveTopology::LineList:
		return D3DPT_LINELIST;
	case EPrimitiveTopology::LineStrip:
		return D3DPT_LINESTRIP;
	case EPrimitiveTopology::TriangleList:
		return D3DPT_TRIANGLELIST;
	case EPrimitiveTopology::TriangleStrip:
		return D3DPT_TRIANGLESTRIP;
	default:
		return D3DPT_TRIANGLELIST;
	}
}
} // namespace

uint32_t CGeometry::GetVertexCount() const
{
	if(m_streams.empty() || !m_streams[0].buffer)
		return 0;
	const uint32_t stride = m_streams[0].buffer->GetStride();
	if(stride == 0)
		return 0;
	return m_streams[0].buffer->GetSizeBytes() / stride;
}

uint32_t CGeometry::GetPrimitiveCount() const
{
	const uint32_t n = m_ib ? GetIndexCount() : GetVertexCount();
	switch(m_topology)
	{
	case EPrimitiveTopology::PointList:
		return n;
	case EPrimitiveTopology::LineList:
		return n / 2;
	case EPrimitiveTopology::LineStrip:
		return (n >= 2) ? (n - 1) : 0;
	case EPrimitiveTopology::TriangleList:
		return n / 3;
	case EPrimitiveTopology::TriangleStrip:
		return (n >= 3) ? (n - 2) : 0;
	default:
		return 0;
	}
}

void CGeometry::Bind(IDirect3DDevice9Ex* device) const
{
	if(!device)
		return;

	if(m_vdecl)
		m_vdecl->Bind(device);

	for(uint32_t s = 0; s < (uint32_t)m_streams.size(); ++s)
	{
		const SStreamBinding& sb = m_streams[s];
		if(!sb.buffer)
			continue;

		sb.buffer->Bind(device, s, sb.offset);

		// D3D9-инстансинг делается через SetStreamSourceFreq
		// if (sb.instanceStepRate > 0)
		//     device->SetStreamSourceFreq(s, D3DSTREAMSOURCE_INSTANCEDATA | sb.instanceStepRate);
	}

	if(m_ib)
		m_ib->Bind(device);
}

void CGeometry::Draw(IDirect3DDevice9Ex* device) const
{
	if(!device)
		return;

	const D3DPRIMITIVETYPE prim = ToD3DPrimitive(m_topology);
	const uint32_t count = GetPrimitiveCount();

	if(m_ib)
	{
		const uint32_t vcount = GetVertexCount();
		device->DrawIndexedPrimitive(prim, 0, 0, vcount, 0, count);
	}
	else
	{
		device->DrawPrimitive(prim, 0, count);
	}
}
////////////////////////////////////////////////////////////////////////////////
