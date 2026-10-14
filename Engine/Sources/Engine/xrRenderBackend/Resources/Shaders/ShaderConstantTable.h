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
#include "ShaderConstant.h"
////////////////////////////////////////////////////////////////////////////////
constexpr uint16_t RC_dest_pixel_bit = (1 << 0);
constexpr uint16_t RC_dest_vertex_bit = (1 << 1);
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderConstantTable
{
  public:
	// Разобрать CTAB-секцию из байткода. destination — RC_dest_vertex / RC_dest_pixel.
	BOOL Parse(const void* bytecode, UINT bytecodeSize, uint16_t destination);

	// Слить в себя другую таблицу (для объединения VS и PS).
	void Merge(const CShaderConstantTable& other);

	void Clear();

	const xr_vector<CShaderConstantEntry>& All() const { return m_constants; }
	const xr_vector<CShaderSamplerEntry>& Samplers() const { return m_samplers; }

	// Поиск по имени.
	const CShaderConstantEntry* FindConstant(LPCSTR name) const;
	const CShaderSamplerEntry* FindSampler(LPCSTR name) const;

	bool empty() const { return m_constants.empty() && m_samplers.empty(); }

  private:
	xr_vector<CShaderConstantEntry> m_constants;
	xr_vector<CShaderSamplerEntry> m_samplers;
};
////////////////////////////////////////////////////////////////////////////////
