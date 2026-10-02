////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRenderBackend/Resources/SharedResource.h>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CVertexDeclaration : public CRefCountedResource
{
public:
	CVertexDeclaration() = default;
	~CVertexDeclaration() override;

	CVertexDeclaration(const CVertexDeclaration&) = delete;
	CVertexDeclaration& operator=(const CVertexDeclaration&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_InputLayoutDesc& desc);

	const RHI_InputLayoutDesc& GetLayout() const { return m_desc; }
	RHI_InputLayoutHandle GetRHIHandle() const { return m_rhiHandle; }

	void Bind(IRenderBackend& rhi) const;

private:
	void DestroyRHI();

	RHI_InputLayoutDesc   m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_InputLayoutHandle m_rhiHandle{};
};
using ref_vertexdecl = CSharedPtr<CVertexDeclaration>;
////////////////////////////////////////////////////////////////////////////////
