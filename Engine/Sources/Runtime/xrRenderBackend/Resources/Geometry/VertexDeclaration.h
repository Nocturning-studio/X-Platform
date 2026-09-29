////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRenderBackend/Resources/DeviceResource.h>
#include "VertexLayout.h"
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CVertexDeclaration : public CDeviceResource
{
public:
	virtual ~CVertexDeclaration() = default;

	const CVertexLayoutDesc& GetLayout() const { return m_layout; }

	// D3D9: SetVertexDeclaration(IDirect3DVertexDeclaration9).
	// DX12: no-op — лэйаут является частью PSO, поэтому Bind() в DX12-пути не вызывается
	virtual void Bind(IDirect3DDevice9Ex* device) const = 0;

protected:
	CVertexLayoutDesc m_layout;
};
using ref_vertexdecl = CSharedPtr<CVertexDeclaration>;
////////////////////////////////////////////////////////////////////////////////
