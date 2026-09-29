////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "Buffer.h"
#include "VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////

enum class EPrimitiveTopology : uint32_t
{
	PointList = 1,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	// PatchList_* зарезервировано под тесселяцию в DX11/12.
};

// API-агностичный handle геометрии. Хранит ссылки на уже созданные
// device-ресурсы (буферы, декларацию) и топологию.
class XRRB_API CGeometry : public CSharedResource
{
  public:
	CGeometry() = default;
	~CGeometry() override = default;

	// --- Vertex declaration ---
	void SetVertexDeclaration(const ref_vertexdecl& d) { m_vdecl = d; }
	const ref_vertexdecl& GetVertexDeclaration() const { return m_vdecl; }

	// --- Vertex streams ---
	struct SStreamBinding
	{
		ref_vertexbuffer buffer;
		uint32_t offset = 0;
		uint32_t instanceStepRate = 0; // 0 = per-vertex, >0 = per-instance
	};

	void SetVertexBuffer(uint32_t stream, const ref_vertexbuffer& vb, uint32_t offset = 0, uint32_t instanceStepRate = 0)
	{
		if(stream >= m_streams.size())
			m_streams.resize(stream + 1);
		m_streams[stream].buffer = vb;
		m_streams[stream].offset = offset;
		m_streams[stream].instanceStepRate = instanceStepRate;
	}

	const ref_vertexbuffer& GetVertexBuffer(uint32_t stream) const
	{
		static const ref_vertexbuffer s_null;
		return (stream < m_streams.size()) ? m_streams[stream].buffer : s_null;
	}
	uint32_t GetVertexBufferOffset(uint32_t stream) const
	{
		return (stream < m_streams.size()) ? m_streams[stream].offset : 0;
	}
	uint32_t GetVertexStreamCount() const { return (uint32_t)m_streams.size(); }

	// --- Index buffer ---
	void SetIndexBuffer(const ref_indexbuffer& ib) { m_ib = ib; }
	const ref_indexbuffer& GetIndexBuffer() const { return m_ib; }

	// --- Topology ---
	void SetTopology(EPrimitiveTopology t) { m_topology = t; }
	EPrimitiveTopology GetTopology() const { return m_topology; }

	// --- Stats ---
	uint32_t GetIndexCount() const { return m_ib ? m_ib->GetIndexCount() : 0; }
	uint32_t GetVertexCount() const;	   // по stream 0
	uint32_t GetPrimitiveCount() const; // по топологии и count

	// --- Binding / Drawing ---
	// Устанавливает декларацию, все потоки и индексный буфер в текущий контекст.
	void Bind(IDirect3DDevice9Ex* device) const;

	// Удобный хелпер. В D3D9 топология передаётся именно здесь (нет topology state).
	// В DX11/12 бекенд сам применит топологию по GetTopology().
	void Draw(IDirect3DDevice9Ex* device) const;

  private:
	xr_vector<SStreamBinding> m_streams;
	ref_indexbuffer m_ib;
	ref_vertexdecl m_vdecl;
	EPrimitiveTopology m_topology = EPrimitiveTopology::TriangleList;
};
using ref_geometry = CSharedPtr<CGeometry>;
////////////////////////////////////////////////////////////////////////////////
