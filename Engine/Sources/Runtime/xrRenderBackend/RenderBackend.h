////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrRenderBackendAPI.h"
#include "States/StateCache.h"
#include "Resources/Textures/Texture.h"
#include "Resources/Shaders/ShaderPass.h"
#include "Resources/ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////
enum class RHI_BackendType : uint32_t;
struct RHI_PresentationParams;
struct RHIDeviceCaps;
class IRenderBackend;
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CRenderBackend
{
  public:
	CRenderBackend();
	~CRenderBackend();

	CRenderBackend(const CRenderBackend&) = delete;
	CRenderBackend& operator=(const CRenderBackend&) = delete;

	// --- RHI lifecycle ---
	bool CreateDevice(HWND hWnd, RHI_BackendType backendType, const RHI_PresentationParams& params);
	void DestroyDevice();

	// Пересоздать swap chain (resize / смена режима) с новыми параметрами.
	bool ResetDevice(const RHI_PresentationParams& params);
	bool NeedReset() const;

	// --- Frame ---
	void BeginFrame();
	void EndFrame();
	void Present();
	bool IsInScene() const { return m_inScene; }

	// --- Device lost / reset ---
	void OnDeviceLost();
	bool OnDeviceReset();

	// --- Device access ---
	IRenderBackend* GetRHI() const { return m_pRHI; }
	IDirect3D9Ex* GetD3D() const { return m_pD3D; }
	IDirect3DDevice9Ex* GetDevice() const { return m_pDevice; }
	IDirect3DSurface9* GetBaseRT() const { return m_pBaseRT; }
	IDirect3DSurface9* GetBaseZB() const { return m_pBaseZB; }
	const D3DPRESENT_PARAMETERS& GetPresentParams() const { return m_DevPP; }

	const RHIDeviceCaps& GetDeviceCaps() const;
	bool IsReady() const { return m_pDevice != nullptr; }

	// --- Subsystems ---
	CResourceManager& Resources() { return m_resources; }
	const CResourceManager& Resources() const { return m_resources; }
	CStateCache& States() { return m_states; }
	const CStateCache& States() const { return m_states; }

	// --- Удобные форвардеры ---
	void SetShaderPass(CShaderPass* pass);
	void SetShaderPass(CShaderPass& pass) { SetShaderPass(&pass); }

	ref_texture CreateRenderTarget(uint32_t w, uint32_t h, ETextureFormat fmt, uint32_t mips = 1);
	ref_texture CreateDepthStencil(uint32_t w, uint32_t h, ETextureFormat fmt = ETextureFormat::D24_UNORM_S8_UINT);
	void Clear(uint32_t flags, D3DCOLOR color = 0, float z = 1.0f, uint32_t stencil = 0);

	ref_vertexdecl CreateVertexDeclaration(const CVertexLayoutDesc& layout);
	ref_vertexbuffer CreateVertexBuffer(const CVertexBufferDesc& desc, const void* data = nullptr);
	ref_indexbuffer CreateIndexBuffer(const CIndexBufferDesc& desc, const void* data = nullptr);
	ref_geometry CreateGeometry();

	void BindGeometry(const ref_geometry& g);
	void DrawGeometry(const ref_geometry& g);

  private:
	bool LoadRHIModule(RHI_BackendType type);
	bool AcquireBackBuffers();
	void ReleaseBackBuffers();
	void ApplyPresentParamsFromRHI(const RHI_PresentationParams& params);
	void SetupDefaultViewport();

  private:
	// RHI
	HMODULE m_hRHI = nullptr;
	IRenderBackend* m_pRHI = nullptr;

	IDirect3D9Ex* m_pD3D = nullptr;
	IDirect3DDevice9Ex* m_pDevice = nullptr;
	IDirect3DSurface9* m_pBaseRT = nullptr;
	IDirect3DSurface9* m_pBaseZB = nullptr;
	D3DPRESENT_PARAMETERS m_DevPP{};

	CResourceManager m_resources;
	CStateCache m_states;
	bool m_inScene = false;
};
////////////////////////////////////////////////////////////////////////////////
