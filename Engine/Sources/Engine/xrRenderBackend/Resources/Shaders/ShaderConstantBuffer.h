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
#include <X-RHI/include/XRHI.h>
////////////////////////////////////////////////////////////////////////////////
class CShaderConstantTable;
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderConstantBuffer
{
  public:
	void Reset();

	bool SetFloat(LPCSTR name, float v, const CShaderConstantTable& table);
	bool SetVector(LPCSTR name, const fvec4& v, const CShaderConstantTable& table);
	bool SetMatrix(LPCSTR name, const fmat4x4& m, const CShaderConstantTable& table);
	bool SetMatrixArray(LPCSTR name, uint32_t index, const fmat4x4& m, const CShaderConstantTable& table);

	// Отправить грязные диапазоны в RHI.
	void Flush(IRenderBackend& rhi);

  private:
	void MarkDirty(bool pixel, uint32_t lo, uint32_t hi);

	// Массивы значений. 256 float4-регистров — максимум SM3.0.
	ALIGN(16)
	fvec4 m_vsData[256];
	ALIGN(16)
	fvec4 m_psData[256];

	bool m_vsDirty = false;
	bool m_psDirty = false;
	uint32_t m_vsDirtyLo = 256;
	uint32_t m_vsDirtyHi = 0;
	uint32_t m_psDirtyLo = 256;
	uint32_t m_psDirtyHi = 0;
};
////////////////////////////////////////////////////////////////////////////////
