////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
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

	// --- Device reset ---
	bool OnDeviceReset();

	// --- Device access ---
	IRenderBackend* GetRHI() const { return m_pRHI; }
	DX_DEPRECATED IDirect3DDevice9Ex* GetDevice() const;
	const RHIDeviceCaps& GetDeviceCaps() const;
	const RHI_PresentationParams& GetPresentParams() const { return m_presentParams; }
	bool IsReady() const { return m_pRHI != nullptr; }
	uint32_t GetBackBufferWidth() const;
	uint32_t GetBackBufferHeight() const;

	// --- Subsystems ---
	CResourceManager& Resources() { return m_resources; }
	const CResourceManager& Resources() const { return m_resources; }
	CStateCache& States() { return m_states; }
	const CStateCache& States() const { return m_states; }

	// --- Удобные форвардеры ---
	void SetShaderPass(CShaderPass* pass);
	void SetShaderPass(CShaderPass& pass) { SetShaderPass(&pass); }

	ref_texture CreateRenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips = 1);
	ref_texture CreateDepthStencil(uint32_t w, uint32_t h, RHI_Format fmt = RHI_Format::D24_UNORM_S8_UINT);
	void Clear(uint32_t flags, uint32_t colorARGB = 0, float z = 1.0f, uint32_t stencil = 0);

	ref_vertexdecl CreateVertexDeclaration(const RHI_InputLayoutDesc& layout);
	ref_vertexbuffer CreateVertexBuffer(const RHI_BufferDesc& desc, const void* data = nullptr);
	ref_indexbuffer CreateIndexBuffer(const RHI_BufferDesc& desc, const void* data = nullptr);
	ref_geometry CreateGeometry();

	void BindGeometry(const ref_geometry& g);
	void DrawGeometry(const ref_geometry& g);

  private:
	bool LoadRHIModule(RHI_BackendType type);

  private:
	// RHI
	HMODULE m_hRHI = nullptr;
	IRenderBackend* m_pRHI = nullptr;

	RHI_PresentationParams m_presentParams{};

	CResourceManager m_resources;
	CStateCache m_states;
	bool m_inScene = false;
};
////////////////////////////////////////////////////////////////////////////////
