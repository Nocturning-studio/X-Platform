////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <DXSDK/d3dx9.h>
#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CShaderIncluder : public ID3DXInclude
{
  public:
	CShaderIncluder();
	~CShaderIncluder();

	// Сбросить состояние для новой компиляции
	void Reset();

	// Дополнительный путь поиска, относительно $engine_shaders$.
	// Trailing separator добавляется автоматически. Пустая строка = корень.
	void AddSearchPath(LPCSTR path);

	// Все файлы, успешно открытые в течение последней компиляции.
	// Пути относительны $engine_shaders$, дедуплицированы, порядок — как они встречались.
	const xr_vector<xr_string>& GetIncludedFiles() const { return m_included; }

	// --- ID3DXInclude ---
	HRESULT __stdcall Open(D3DXINCLUDE_TYPE type,
						   LPCSTR pName,
						   LPCVOID pParentData,
						   LPCVOID* ppData,
						   UINT* pBytes) override;

	HRESULT __stdcall Close(LPCVOID pData) override;

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
