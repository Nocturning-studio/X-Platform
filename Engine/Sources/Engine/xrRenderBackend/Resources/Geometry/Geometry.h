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
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/Resources/SharedResource.h>
#include "Buffer.h"
#include "VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CGeometry : public CRefCountedResource
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
		uint32_t instanceStepRate = 0;
	};

	void SetVertexBuffer(uint32_t stream, const ref_vertexbuffer& vb,
						 uint32_t offset = 0, uint32_t instanceStepRate = 0)
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
	void SetTopology(RHI_Topology t) { m_topology = t; }
	RHI_Topology GetTopology() const { return m_topology; }

	// --- Stats ---
	uint32_t GetIndexCount() const { return m_ib ? m_ib->GetIndexCount() : 0; }
	uint32_t GetVertexCount() const; // по stream 0

	// --- Binding / Drawing ---
	// Устанавливает декларацию, все потоки, индексный буфер и топологию.
	void Bind(IRenderBackend& rhi) const;

	// Вызывает Draw/DrawIndexed. В D3D9 топология уже была применена
	// внутри Bind через SetPrimitiveTopology.
	void Draw(IRenderBackend& rhi) const;

  private:
	xr_vector<SStreamBinding> m_streams;
	ref_indexbuffer m_ib;
	ref_vertexdecl m_vdecl;
	RHI_Topology m_topology = RHI_Topology::TriangleList;
};
using ref_geometry = CSharedPtr<CGeometry>;
////////////////////////////////////////////////////////////////////////////////
