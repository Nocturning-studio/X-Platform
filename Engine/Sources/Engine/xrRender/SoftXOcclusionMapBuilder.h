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
class CHOM;
class SoftXOcclusionCore;

class SoftXOcclusionMapBuilder
{
  public:
	SoftXOcclusionMapBuilder() = default;
	~SoftXOcclusionMapBuilder() = default;

	// Загрузить геометрию из CHOM (вершины + индексы)
	void Load(const CHOM& hom);

	// Освободить ресурсы
	void Unload();

	bool IsLoaded() const { return m_loaded; }

	// Привязать ядро SoftX
	void SetCore(SoftXOcclusionCore* core) { m_core = core; }

	// Асинхронно заполнить depth-буфер (write-буфер ядра)
	void Build();

	u32 GetVertexCount() const { return m_vertexCount; }
	u32 GetIndexCount() const { return m_indexCount; }

  private:
	SoftXOcclusionCore* m_core = nullptr;

	// Геометрия окклюдеров HOM
	std::unique_ptr<SoftX::VertexBuffer> m_occluderVB;
	std::unique_ptr<SoftX::IndexBuffer> m_occluderIB;

	u32 m_vertexCount = 0;
	u32 m_indexCount = 0;
	bool m_loaded = false;

	// Вспомогательные функции для извлечения геометрии из CDB
	void ExtractGeometry(const CDB::MODEL* model,
						 xr_vector<fvec3>& outVertices,
						 xr_vector<u16>& outIndices) const;
};
////////////////////////////////////////////////////////////////////////////////
