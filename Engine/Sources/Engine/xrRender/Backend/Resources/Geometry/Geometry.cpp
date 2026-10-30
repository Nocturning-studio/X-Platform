////////////////////////////////////////////////////////////////////////////////
// Author: NS_Deathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Geometry.h"
////////////////////////////////////////////////////////////////////////////////
uint32_t CGeometry::GetVertexCount() const
{
	if(m_streams.empty() || !m_streams[0].buffer)
		return 0;
	const uint32_t stride = m_streams[0].buffer->GetStride();
	if(stride == 0)
		return 0;
	return m_streams[0].buffer->GetSizeBytes() / stride;
}

void CGeometry::Bind(IRenderBackend& rhi) const
{
	// Vertex streams.
	for(uint32_t s = 0; s < (uint32_t)m_streams.size(); ++s)
	{
		const SStreamBinding& sb = m_streams[s];
		if(!sb.buffer)
			continue;
		sb.buffer->Bind(rhi, s, sb.offset);
	}

	// Index buffer.
	if(m_ib)
		m_ib->Bind(rhi);

	// Topology (динамический стейт).
	rhi.SetPrimitiveTopology(m_topology);
}

void CGeometry::Draw(IRenderBackend& rhi) const
{
	if(m_ib)
		rhi.DrawIndexed(GetIndexCount(), 0, 0);
	else
		rhi.Draw(GetVertexCount(), 0);
}
////////////////////////////////////////////////////////////////////////////////
