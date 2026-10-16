////////////////////////////////////////////////////////////////////////////////
// Created: 08.10.2026 23:10:10
// Author: NS_Deathman
// File: XRHI_PipelineState.h
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
struct RHI_PipelineStateDesc
{
	static constexpr uint32_t kMaxRenderTargets = 8;

	// ---- Shaders ----
	RHI_ShaderHandle vertexShader{};
	RHI_ShaderHandle pixelShader{};

	// ---- Root signature / register layout ----
	RHI_PipelineLayoutHandle layout{};

	// ---- Input assembly ----
	RHI_InputLayoutHandle inputLayout{};
	RHI_PrimitiveTopologyType topologyType = RHI_PrimitiveTopologyType::Triangle;

	// ---- Fixed-function state ----
	RHI_BlendState blend{}; // RT0 (совместимо с D3D9)

	// D3D12 supports per-RT independent blend. Если false — blend для всех.
	bool independentBlend = false;
	RHI_BlendState blendPerRT[kMaxRenderTargets]{};

	RHI_DepthStencilState depthStencil{};
	RHI_RasterizerState rasterizer{};

	// ---- Render target / depth-stencil formats ----
	RHI_Format rtvFormats[kMaxRenderTargets] = {};
	uint32_t rtvCount = 0;
	RHI_Format dsvFormat = RHI_Format::Unknown;

	// ---- MSAA ----
	RHI_SampleDesc sample{};

	// ---- Depth bounds test (enable — в PSO; значения — динамически) ----
	bool depthBoundsTestEnable = false;

	// ---- Debug ----
	LPCSTR debugName = "PSO";

	// ---- Удобные фабрики ----
	static RHI_PipelineStateDesc Default(RHI_ShaderHandle vs,
										 RHI_ShaderHandle ps,
										 RHI_InputLayoutHandle il,
										 RHI_PipelineLayoutHandle layout,
										 RHI_Format rtFmt,
										 RHI_Format dsFmt = RHI_Format::D24_UNORM_S8_UINT)
	{
		RHI_PipelineStateDesc d;
		d.vertexShader = vs;
		d.pixelShader = ps;
		d.inputLayout = il;
		d.layout = layout;
		d.rtvFormats[0] = rtFmt;
		d.rtvCount = 1;
		d.dsvFormat = dsFmt;
		return d;
	}

	constexpr bool operator==(const RHI_PipelineStateDesc& o) const noexcept
	{
		if(vertexShader != o.vertexShader)
			return false;
		if(pixelShader != o.pixelShader)
			return false;
		if(layout != o.layout)
			return false;
		if(inputLayout != o.inputLayout)
			return false;
		if(topologyType != o.topologyType)
			return false;
		if(!(blend == o.blend))
			return false;
		if(independentBlend != o.independentBlend)
			return false;
		if(independentBlend)
		{
			for(uint32_t i = 0; i < rtvCount; ++i)
				if(!(blendPerRT[i] == o.blendPerRT[i]))
					return false;
		}
		if(!(depthStencil == o.depthStencil))
			return false;
		if(!(rasterizer == o.rasterizer))
			return false;
		if(rtvCount != o.rtvCount)
			return false;
		for(uint32_t i = 0; i < rtvCount; ++i)
			if(rtvFormats[i] != o.rtvFormats[i])
				return false;
		if(dsvFormat != o.dsvFormat)
			return false;
		if(!(sample == o.sample))
			return false;
		if(depthBoundsTestEnable != o.depthBoundsTestEnable)
			return false;
		return true;
	}
	constexpr bool operator!=(const RHI_PipelineStateDesc& o) const noexcept { return !(*this == o); }
};

// ============================================================================
// Хэш (для кэширования PSO в бэкендах)
// ============================================================================
namespace std
{
namespace detail
{
inline void HashMix(uint64_t& h, uint64_t v) noexcept
{
	h ^= v;
	h *= 1099511628211ull;
}

inline void HashMixFloat(uint64_t& h, float f) noexcept
{
	uint32_t bits;
	std::memcpy(&bits, &f, 4);
	HashMix(h, bits);
}

inline void HashMixBlend(uint64_t& h, const RHI_BlendState& b) noexcept
{
	HashMix(h, static_cast<uint64_t>(b.enable));
	HashMix(h, static_cast<uint64_t>(b.srcColor) | (uint64_t(b.dstColor) << 16));
	HashMix(h, static_cast<uint64_t>(b.opColor) | (uint64_t(b.writeMask) << 16));
	HashMix(h, static_cast<uint64_t>(b.separateAlpha));
	HashMix(h, static_cast<uint64_t>(b.srcAlpha) | (uint64_t(b.dstAlpha) << 16));
	HashMix(h, static_cast<uint64_t>(b.opAlpha));
}

inline void HashMixDepthStencil(uint64_t& h, const RHI_DepthStencilState& d) noexcept
{
	HashMix(h, static_cast<uint64_t>(d.depthEnable) |
				   (uint64_t(d.depthWriteEnable) << 8) |
				   (uint64_t(d.depthFunc) << 16) |
				   (uint64_t(d.stencilEnable) << 24));
	if(d.stencilEnable)
	{
		HashMix(h, static_cast<uint64_t>(d.stencilFunc) |
					   (uint64_t(d.stencilReadMask) << 16) |
					   (uint64_t(d.stencilWriteMask) << 24) |
					   (uint64_t(d.stencilFailOp) << 32) |
					   (uint64_t(d.stencilDepthFailOp) << 40) |
					   (uint64_t(d.stencilPassOp) << 48));
	}
}

inline void HashMixRasterizer(uint64_t& h, const RHI_RasterizerState& r) noexcept
{
	HashMix(h, static_cast<uint64_t>(r.fillMode) |
				   (uint64_t(r.cullMode) << 8) |
				   (uint64_t(r.scissorEnable) << 16) |
				   (uint64_t(r.frontCounterClockwise) << 24));
	HashMixFloat(h, r.depthBias);
	HashMixFloat(h, r.slopeScaledDepthBias);
}
} // namespace detail

template <>
struct hash<RHI_PipelineStateDesc>
{
	size_t operator()(const RHI_PipelineStateDesc& d) const noexcept
	{
		uint64_t h = 1469598103934665603ull;

		detail::HashMix(h, d.vertexShader.id);
		detail::HashMix(h, d.pixelShader.id);
		detail::HashMix(h, d.layout.id);
		detail::HashMix(h, d.inputLayout.id);
		detail::HashMix(h, static_cast<uint64_t>(d.topologyType));

		detail::HashMixBlend(h, d.blend);

		detail::HashMix(h, static_cast<uint64_t>(d.independentBlend));
		if(d.independentBlend)
		{
			for(uint32_t i = 0; i < d.rtvCount && i < RHI_PipelineStateDesc::kMaxRenderTargets; ++i)
				detail::HashMixBlend(h, d.blendPerRT[i]);
		}

		detail::HashMixDepthStencil(h, d.depthStencil);
		detail::HashMixRasterizer(h, d.rasterizer);

		detail::HashMix(h, d.rtvCount);
		for(uint32_t i = 0; i < d.rtvCount; ++i)
			detail::HashMix(h, static_cast<uint64_t>(d.rtvFormats[i]));
		detail::HashMix(h, static_cast<uint64_t>(d.dsvFormat));

		detail::HashMix(h, static_cast<uint64_t>(d.sample.count) |
							   (uint64_t(d.sample.quality) << 16));
		detail::HashMix(h, static_cast<uint64_t>(d.depthBoundsTestEnable));

		return static_cast<size_t>(h);
	}
};
} // namespace std
////////////////////////////////////////////////////////////////////////////////
