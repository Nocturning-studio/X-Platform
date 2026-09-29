////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "Buffer.h"
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CD3D9VertexBuffer : public CVertexBuffer
{
  public:
	CD3D9VertexBuffer() = default;
	~CD3D9VertexBuffer() override;

	HRESULT Create(IDirect3DDevice9Ex* device, const CVertexBufferDesc& desc);

	void* Lock(uint32_t offset, uint32_t size, uint32_t flags) override;
	void Unlock() override;

	void Bind(IDirect3DDevice9Ex* device, uint32_t stream, uint32_t offset) const override;

	void OnDeviceLost() override;
	HRESULT OnDeviceReset(IDirect3DDevice9Ex* device) override;

	IDirect3DVertexBuffer9* GetD3D9Buffer() const { return m_vb; }

  private:
	IDirect3DVertexBuffer9* m_vb = nullptr;
	D3DPOOL m_pool = D3DPOOL_MANAGED;
	bool m_locked = false;
};

////////////////////////////////////////////////////////////////////////////////

class XRRB_API CD3D9IndexBuffer : public CIndexBuffer
{
  public:
	CD3D9IndexBuffer() = default;
	~CD3D9IndexBuffer() override;

	HRESULT Create(IDirect3DDevice9Ex* device, const CIndexBufferDesc& desc);

	void* Lock(uint32_t offset, uint32_t size, uint32_t flags) override;
	void Unlock() override;

	void Bind(IDirect3DDevice9Ex* device) const override;

	void OnDeviceLost() override;
	HRESULT OnDeviceReset(IDirect3DDevice9Ex* device) override;

	IDirect3DIndexBuffer9* GetD3D9Buffer() const { return m_ib; }

  private:
	IDirect3DIndexBuffer9* m_ib = nullptr;
	D3DPOOL m_pool = D3DPOOL_MANAGED;
	bool m_locked = false;
};
////////////////////////////////////////////////////////////////////////////////
