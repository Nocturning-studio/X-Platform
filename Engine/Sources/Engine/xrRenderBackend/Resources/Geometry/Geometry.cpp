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
