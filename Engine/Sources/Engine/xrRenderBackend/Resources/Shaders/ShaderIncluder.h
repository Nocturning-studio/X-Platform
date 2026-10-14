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
#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderIncluder : public RHI_IncludeHandler
{
  public:
	CShaderIncluder();
	~CShaderIncluder();

	void Reset();
	void AddSearchPath(LPCSTR path);

	const xr_vector<xr_string>& GetIncludedFiles() const { return m_included; }

	bool Open(RHI_IncludeType type, const char* name, const void* parentData, const void** outData, size_t* outSize) override;
	void Close(const void* data) override;

  private:
	xr_string MakeGuardName(const xr_string& path) const;

  private:
	std::deque<xr_string> m_buffers;

	xr_vector<xr_string> m_searchPaths;
	xr_vector<xr_string> m_included;
	std::unordered_set<xr_string> m_includedSet;
	bool m_wrapWithGuard = true;
};
////////////////////////////////////////////////////////////////////////////////
