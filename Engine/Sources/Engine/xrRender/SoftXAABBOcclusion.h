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
////////////////////////////////////////////////////////////////////////////////
class SoftXOcclusionCore;

class SoftXAABBOcclusion
{
  public:
	SoftXAABBOcclusion() = default;
	~SoftXAABBOcclusion() = default;

	void Initialize(SoftXOcclusionCore* core);
	void Shutdown();

	// Тест видимости AABB в мировых координатах
	// depthWidth/Height – размер depth‑буфера, viewProj – матрица View*Projection
	bool TestAABB(const Fbox3& worldAABB, const fmat4x4& viewProj, uint2 depthResolution) const;

	// Тест для экранного прямоугольника (координаты 0..1, как в CHOM)
	bool TestRect(float x0, float y0, float x1, float y1, float depth, uint2 depthResolution) const;

	// Тест для полигона в мировых координатах
	bool TestPolygon(const sPoly& worldPoly, const fmat4x4& viewProj, uint2 depthResolution) const;

  private:
	SoftXOcclusionCore* m_core = nullptr;

	// Вспомогательные методы для проекции вершин
	struct ProjectedVertex
	{
		float x, y, z; // экранные 0..1 (x,y), глубина 0..1 (z)
		bool valid;	   // false если за near plane
	};
	ProjectedVertex Project(const fvec3& worldPos, const fmat4x4& viewProj) const;
};
////////////////////////////////////////////////////////////////////////////////
