////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
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
