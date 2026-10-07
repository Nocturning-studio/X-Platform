////////////////////////////////////////////////////////////////////////////////
// Created: 07.10.2026 19:49:37
// Author: NS_Deathman
// File: xrBackendDX12.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include <d3d12.h>
#include <dxgi1_6.h>
////////////////////////////////////////////////////////////////////////////////
class XRRHI_API CRenderBackendDX12 : public IRenderBackend
{
  public:
	CRenderBackendDX12();
	virtual ~CRenderBackendDX12();

	// ============================================================================
	// Lifecycle / presentation
	// ============================================================================
	virtual bool CreateDevice(HWND hWnd, const RHI_PresentationParams& params) override;
	virtual void DestroyDevice() override;
	virtual bool Reset(const RHI_PresentationParams& params) override;
	virtual RHI_DeviceStatus CheckDeviceStatus() const override;
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	virtual void* GetDeviceHandle() override { return m_pDevice; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const override { return m_DeviceCaps; }

	virtual uint32_t GetBackBufferWidth() const override { return m_backBufferWidth; }
	virtual uint32_t GetBackBufferHeight() const override { return m_backBufferHeight; }
	virtual RHI_Format GetBackBufferFormat() const override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const override;

	// ============================================================================
	// The following are not implemented in this first iteration — see .cpp
	// for stub bodies. They return empty handles / false / no-op so the class
	// links and the engine can at least create a device and present.
	// ============================================================================
	virtual RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0) override;
	virtual RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0) override;
	virtual RHI_RenderTargetView GetBackBufferRTV() const override;
	virtual RHI_DepthStencilView GetBackBufferDSV() const override;
	virtual void DestroyRTV(RHI_RenderTargetView rtv) override;
	virtual void DestroyDSV(RHI_DepthStencilView dsv) override;

	virtual void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv) override;
	virtual void ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color) override;
	virtual void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil) override;
	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) override;

	virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) override;
	virtual void DestroyTexture(RHI_TextureHandle handle) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) override;

	virtual RHI_BufferHandle CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) override;
	virtual RHI_BufferHandle CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) override;
	virtual void DestroyBuffer(RHI_BufferHandle handle) override;

	virtual void* LockBuffer(RHI_BufferHandle handle, uint32_t offset, uint32_t size, uint32_t flags) override;
	virtual void UnlockBuffer(RHI_BufferHandle handle) override;

	virtual RHI_InputLayoutHandle CreateInputLayout(const RHI_InputLayoutDesc& desc) override;
	virtual void DestroyInputLayout(RHI_InputLayoutHandle handle) override;

	virtual void SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb, uint32_t offset, uint32_t stride) override;
	virtual void SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt) override;
	virtual void SetInputLayout(RHI_InputLayoutHandle layout) override;
	virtual void SetPrimitiveTopology(RHI_Topology topology) override;

	virtual void SetBlendState(const RHI_BlendState& state) override;
	virtual const RHI_BlendState& GetBlendState() const override { return m_blendCache; }
	virtual void SetDepthStencilState(const RHI_DepthStencilState& state) override;
	virtual const RHI_DepthStencilState& GetDepthStencilState() const override { return m_depthCache; }
	virtual void SetRasterizerState(const RHI_RasterizerState& state) override;
	virtual const RHI_RasterizerState& GetRasterizerState() const override { return m_rasterCache; }

	virtual void SetViewport(const RHI_Viewport& vp) override;
	virtual void SetScissorRect(const RHI_Rect* rect) override;
	virtual RHI_Viewport GetViewport() const override { return m_viewportCache; }
	virtual bool GetScissorRect(RHI_Rect& out) const override;

	virtual void InvalidateStateCache() override;

	virtual RHI_ShaderCompileResult CompileShader(const RHI_ShaderCompileDesc& desc) override;
	virtual bool GetShaderBytecode(RHI_ShaderHandle handle, const void** outData, size_t* outSize) override;
	virtual RHI_ShaderHandle CreateVertexShader(const void* bytecode, size_t size, const char* debugName = nullptr) override;
	virtual RHI_ShaderHandle CreatePixelShader(const void* bytecode, size_t size, const char* debugName = nullptr) override;
	virtual void SetVertexShader(RHI_ShaderHandle handle) override;
	virtual void SetPixelShader(RHI_ShaderHandle handle) override;
	virtual void SetSampler(uint32_t slot, const RHI_SamplerDesc& desc) override;
	virtual void SetShaderResource(uint32_t slot, RHI_TextureHandle tex) override;
	virtual void SetShaderConstants(RHI_ShaderType stage, uint32_t startRegister,
									const float* data, uint32_t vec4Count) override;
	virtual void DestroyShader(RHI_ShaderHandle handle) override;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t baseVertex = 0) override;
	virtual void DrawFullscreen() override;

  private:
	// ============================================================================
	// Creation helpers
	// ============================================================================
	bool CreateFactory(bool enableDebug);
	bool SelectAdapter(IDXGIAdapter4** outAdapter);
	bool CreateD3D12Device(IDXGIAdapter4* adapter, bool enableDebug);
	bool CreateCommandQueue();
	bool CreateSwapChain(HWND hWnd, const RHI_PresentationParams& params);
	bool CreateRTVHeapAndBackBuffers();
	bool CreateFrameSyncObjects();

	// ============================================================================
	// Frame helpers
	// ============================================================================
	void FlushGpu();		// signal + wait for all in-flight frames
	void MoveToNextFrame(); // fence throttle после Present
	void WaitForFrame(uint32_t frameIdx);

	void CacheDeviceCaps();
	void ReleaseSwapChainResources(); // back buffers + RTV heap (device alive)
	void ReleaseFrameResources();	  // allocators, list, fence, event

	// ============================================================================
	// Core objects
	// ============================================================================
	IDXGIFactory6* m_pFactory = nullptr;
	IDXGIAdapter4* m_pAdapter = nullptr;
	ID3D12Device* m_pDevice = nullptr;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	IDXGISwapChain4* m_pSwapChain = nullptr;
	HWND m_hWnd = nullptr;

	// ============================================================================
	// Swap chain / back buffers
	// ============================================================================
	uint32_t m_backBufferWidth = 0;
	uint32_t m_backBufferHeight = 0;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_UNKNOWN;
	uint32_t m_syncInterval = 1;
	bool m_windowed = true;

	static constexpr uint32_t kMaxBackBuffers = 3;
	ID3D12Resource* m_backBuffers[kMaxBackBuffers] = {};
	uint32_t m_backBufferCount = 0;

	// RTV heap для back buffers. Slot 0..N-1 — back buffers.
	ID3D12DescriptorHeap* m_rtvHeap = nullptr;
	uint32_t m_rtvDescriptorSize = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_backBufferRTVHandles[kMaxBackBuffers] = {};

	// Публичные хэндлы back-buffer RTV/DSV (пока — только RTV).
	RHI_RenderTargetView m_backBufferRTV{};
	RHI_DepthStencilView m_backBufferDSV{};

	// ============================================================================
	// Frame sync
	// ============================================================================
	static constexpr uint32_t kFrameCount = 2;

	ID3D12CommandAllocator* m_commandAllocators[kFrameCount] = {};
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	ID3D12Fence* m_fence = nullptr;
	HANDLE m_fenceEvent = nullptr;
	UINT64 m_fenceValues[kFrameCount] = {};

	uint32_t m_frameIndex = 0;
	bool m_inFrame = false;

	// ============================================================================
	// Cached state (см. стубы — храним, чтобы геттеры что-то возвращали)
	// ============================================================================
	RHI_BlendState m_blendCache{};
	RHI_DepthStencilState m_depthCache{};
	RHI_RasterizerState m_rasterCache{};
	RHI_Viewport m_viewportCache{};
	RHI_Rect m_scissorCache{};
	bool m_scissorEnabled = false;

	RHIDeviceCaps m_DeviceCaps;
};
////////////////////////////////////////////////////////////////////////////////
