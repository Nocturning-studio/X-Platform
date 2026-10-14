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
#include "ShaderIncluder.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
constexpr const char* kShaderRoot = "$engine_shaders$";

xr_string NormalizeRelative(const xr_string& in)
{
	xr_string out;
	out.reserve(in.size());

	bool prevSlash = false;
	for(char c : in)
	{
		const bool isSlash = (c == '\\' || c == '/');
		if(isSlash)
		{
			if(prevSlash)
				continue;
			prevSlash = true;
			out += '\\';
		}
		else
		{
			prevSlash = false;
			out += c;
		}
	}

	if(!out.empty() && out.front() == '\\')
		out.erase(out.begin());

	return out;
}
} // namespace

CShaderIncluder::CShaderIncluder()
{
	Reset();
}

CShaderIncluder::~CShaderIncluder() = default;

void CShaderIncluder::Reset()
{
	m_buffers.clear();
	m_searchPaths.clear();
	m_included.clear();
	m_includedSet.clear();
	AddSearchPath("");
}

void CShaderIncluder::AddSearchPath(LPCSTR path)
{
	if(!path)
		return;

	xr_string p = path;
	if(!p.empty() && p.back() != '\\' && p.back() != '/')
		p += '\\';

	m_searchPaths.push_back(std::move(p));
}

bool CShaderIncluder::Open(RHI_IncludeType /*type*/,
						   const char* pName,
						   const void* /*pParentData*/,
						   const void** ppData,
						   size_t* pBytes)
{
	if(!pName || !pName[0] || !ppData || !pBytes)
		return false;

	xr_string resolved;
	IReader* reader = nullptr;

	for(const auto& sp : m_searchPaths)
	{
		const xr_string cand = NormalizeRelative(sp + pName);

		string_path full_path;
		FS.update_path(full_path, kShaderRoot, cand.c_str());

		reader = FS.r_open(full_path);
		if(reader)
		{
			resolved = cand;
			break;
		}
	}

	if(!reader)
	{
		Msg("! [Includer] Cannot resolve '%s' in $engine_shaders$", pName);
		return false;
	}

	xr_string content;
	content.reserve(reader->length() + 256);

	const xr_string guard = MakeGuardName(resolved);

	if(m_wrapWithGuard)
	{
		content += "#ifndef ";
		content += guard;
		content += "\n";
		content += "#define ";
		content += guard;
		content += "\n";
	}

	content.append(static_cast<const char*>(reader->pointer()),
				   static_cast<size_t>(reader->length()));

	if(m_wrapWithGuard)
	{
		content += "\n#endif // ";
		content += guard;
		content += "\n";
	}

	FS.r_close(reader);

	if(m_includedSet.insert(resolved).second)
		m_included.push_back(resolved);

	m_buffers.push_back(std::move(content));
	const xr_string& stored = m_buffers.back();

	*ppData = stored.data();
	*pBytes = stored.size();

	return true;
}

void CShaderIncluder::Close(const void* /*pData*/)
{
}

xr_string CShaderIncluder::MakeGuardName(const xr_string& path) const
{
	xr_string guard;
	guard.reserve(path.size() + 8);
	guard += "_xrinc_";

	for(char c : path)
	{
		if((c >= '0' && c <= '9') ||
		   (c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z'))
			guard += c;
		else
			guard += '_';
	}

	return guard;
}
////////////////////////////////////////////////////////////////////////////////
