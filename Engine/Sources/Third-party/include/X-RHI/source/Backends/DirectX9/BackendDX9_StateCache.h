////////////////////////////////////////////////////////////////////////////////
// Created: 17.10.2026 23:17:19
// Author: NS_Deathman
// File: BackendDX9_StateCache.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class DX9StateCache : public RHI_DynamicStateCache
{
public:
	// Полный сброс — при device reset / destroy
	void Invalidate()
	{
		RHI_DynamicStateCache::Invalidate();

		m_blend = RHI_BlendState{};
		m_depth = RHI_DepthStencilState{};
		m_raster = RHI_RasterizerState{};

		m_blendValid = false;
		m_depthValid = false;
		m_rasterValid = false;

		m_currentVS = nullptr;
		m_currentPS = nullptr;

		m_currentPSO = {};
		m_psoValid = false;
	}

	// ========================================================================
	// D3D9-specific: применяют RHI_*State на устройство с дедупликацией
	// ========================================================================
	void ApplyBlend(IDirect3DDevice9Ex* device, const RHI_BlendState& s);
	void ApplyDepthStencil(IDirect3DDevice9Ex* device, const RHI_DepthStencilState& s);
	void ApplyRasterizer(IDirect3DDevice9Ex* device, const RHI_RasterizerState& s);

	void ApplyViewport(IDirect3DDevice9Ex* device, const RHI_Viewport& vp);
	void ApplyScissorRect(IDirect3DDevice9Ex* device, const RHI_Rect* rect);
	void ApplyStencilRef(IDirect3DDevice9Ex* device, uint8_t ref);
	void ApplyBlendFactor(IDirect3DDevice9Ex* device, const Float4& factor);

	// ========================================================================
	// Shader / PSO bind tracking
	// ========================================================================
	bool NeedsVertexShader(IDirect3DVertexShader9* native) const { return m_currentVS != native; }
	void MarkVertexShader(IDirect3DVertexShader9* native) { m_currentVS = native; }
	IDirect3DVertexShader9* GetVertexShader() const { return m_currentVS; }

	bool NeedsPixelShader(IDirect3DPixelShader9* native) const { return m_currentPS != native; }
	void MarkPixelShader(IDirect3DPixelShader9* native) { m_currentPS = native; }
	IDirect3DPixelShader9* GetPixelShader() const { return m_currentPS; }

	// Вызывается из DestroyShader: если разрушаемый шейдер — текущий, сбрасываем биндинг и PSO-кэш
	void InvalidateVertexShader(IDirect3DVertexShader9* native);
	void InvalidatePixelShader(IDirect3DPixelShader9* native);

	bool NeedsPipelineState(RHI_PipelineStateHandle h) const { return !m_psoValid || m_currentPSO != h; }
	void MarkPipelineState(RHI_PipelineStateHandle h) { m_currentPSO = h; m_psoValid = true; }
	RHI_PipelineStateHandle GetPipelineState() const { return m_psoValid ? m_currentPSO : RHI_PipelineStateHandle{}; }
	void InvalidatePipelineState() { m_currentPSO = {}; m_psoValid = false; }
	void InvalidateRasterizer() { m_rasterValid = false; }

	// Кэш "какой PSO сейчас связан" становится невалидным при любом изменении, которое PSO не покрывает
	bool IsPipelineStateValid() const { return m_psoValid; }

	bool IsScissorEnabled() const { return m_rasterValid && m_raster.scissorEnable; }

private:
	// Blend
	RHI_BlendState m_blend{};
	bool m_blendValid = false;

	// Depth / stencil
	RHI_DepthStencilState m_depth{};
	bool m_depthValid = false;

	// Rasterizer
	RHI_RasterizerState m_raster{};
	bool m_rasterValid = false;

	// Shaders
	IDirect3DVertexShader9* m_currentVS = nullptr;
	IDirect3DPixelShader9* m_currentPS = nullptr;

	// Pipeline
	RHI_PipelineStateHandle m_currentPSO{};
	bool m_psoValid = false;
};
////////////////////////////////////////////////////////////////////////////////
