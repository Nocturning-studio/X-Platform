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
#include <SoftX/include/SoftX.h>
#include "xrFramework/du_cone.h"
#include "xrFramework/du_sphere.h"
#include "xrFramework/du_sphere_part.h"
////////////////////////////////////////////////////////////////////////////////
class light;
class SoftXOcclusionCore;

class SoftXLightVolumeOcclusion
{
  public:
	static constexpr SoftX::OcclusionQuery::queryID kInvalidQueryId = 0xFFFFFFFFu;

	SoftXLightVolumeOcclusion() = default;
	~SoftXLightVolumeOcclusion() = default;

	void Initialize(SoftXOcclusionCore* core);
	void Shutdown();

	void BeginQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport);
	void EndQueries();
	bool IsQueryReady() const;
	void ResetPendingQuery();
	uint32_t GetVisibleSamples(uint32_t queryId) const;

	SoftX::OcclusionQuery::queryID AddVolume(light* L);
	SoftX::OcclusionQuery* GetPendingQuery() const { return m_pendingQuery; }

  private:
	SoftXOcclusionCore* m_core = nullptr;

	// Геометрия объёмов источников
	std::unique_ptr<SoftX::VertexBuffer> m_pointVB;
	std::unique_ptr<SoftX::IndexBuffer> m_pointIB;
	std::unique_ptr<SoftX::VertexBuffer> m_spotVB;
	std::unique_ptr<SoftX::IndexBuffer> m_spotIB;
	std::unique_ptr<SoftX::VertexBuffer> m_omniVB;
	std::unique_ptr<SoftX::IndexBuffer> m_omniIB;

	// Пул запросов
	static constexpr int QUERY_POOL_SIZE = 2;
	std::vector<std::unique_ptr<SoftX::OcclusionQuery>> m_queryPool;
	int m_currentQueryIndex = 0;
	SoftX::OcclusionQuery* m_activeQuery = nullptr;
	SoftX::OcclusionQuery* m_pendingQuery = nullptr;

	// Текущие параметры камеры
	fmat4x4 m_currentViewProj;
	SoftX::Viewport m_currentViewport;

	void CreateGeometries();
	void DestroyGeometries();
};
////////////////////////////////////////////////////////////////////////////////
