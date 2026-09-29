////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CD3D9VertexDeclaration : public CVertexDeclaration
{
  public:
	CD3D9VertexDeclaration() = default;
	~CD3D9VertexDeclaration() override;

	HRESULT Create(IDirect3DDevice9Ex* device, const CVertexLayoutDesc& layout);

	void Bind(IDirect3DDevice9Ex* device) const override;

	void OnDeviceLost() override;
	HRESULT OnDeviceReset(IDirect3DDevice9Ex* device) override;

	IDirect3DVertexDeclaration9* GetD3D9Declaration() const { return m_decl; }

  private:
	IDirect3DVertexDeclaration9* m_decl = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
