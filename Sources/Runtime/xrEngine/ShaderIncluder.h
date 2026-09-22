////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <d3dcompiler.h>
#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CShaderIncluder : public ID3DInclude
{
  public:
	CShaderIncluder();
	~CShaderIncluder();

	void Reset();
	void AddSearchPath(LPCSTR path);

	const xr_vector<xr_string>& GetIncludedFiles() const { return m_included; }

	// --- ID3DInclude ---
	HRESULT __stdcall Open(D3D_INCLUDE_TYPE type,
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
