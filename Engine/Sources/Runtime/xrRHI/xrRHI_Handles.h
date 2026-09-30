////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
template <typename Tag>
struct RHI_Handle
{
	using TagType = Tag;
	static constexpr u32 kInvalidId = u32(-1);

	u32 id = kInvalidId;

	constexpr bool IsValid() const noexcept { return id != kInvalidId; }

	constexpr bool operator==(const RHI_Handle& o) const noexcept { return id == o.id; }
	constexpr bool operator!=(const RHI_Handle& o) const noexcept { return id != o.id; }
	constexpr bool operator< (const RHI_Handle& o) const noexcept { return id < o.id; }

	constexpr explicit operator bool() const noexcept { return IsValid(); }
};

// --- Ресурсы ---
struct RHI_TextureTag {};
struct RHI_BufferTag {};
struct RHI_ConstantBufferTag {};

// --- Пайплайн ---
struct RHI_ShaderTag {};
struct RHI_InputLayoutTag {};
struct RHI_SamplerTag {};
struct RHI_PipelineStateTag {};

// --- Views ---
struct RHI_RenderTargetViewTag {};
struct RHI_DepthStencilViewTag {};
struct RHI_ShaderResourceViewTag {};
struct RHI_UnorderedAccessViewTag {};

// --- Синхронизация ---
struct RHI_QueryTag {};
struct RHI_FenceTag {};

// ============================================================================
// Публичные алиасы
// ============================================================================

// Ресурсы
using RHI_TextureHandle = RHI_Handle<RHI_TextureTag>;
using RHI_BufferHandle = RHI_Handle<RHI_BufferTag>;
using RHI_ConstantBufferHandle = RHI_Handle<RHI_ConstantBufferTag>;

// Пайплайн
using RHI_ShaderHandle = RHI_Handle<RHI_ShaderTag>;
using RHI_InputLayoutHandle = RHI_Handle<RHI_InputLayoutTag>;
using RHI_SamplerHandle = RHI_Handle<RHI_SamplerTag>;
using RHI_PipelineStateHandle = RHI_Handle<RHI_PipelineStateTag>;

// Views
using RHI_RenderTargetView = RHI_Handle<RHI_RenderTargetViewTag>;
using RHI_DepthStencilView = RHI_Handle<RHI_DepthStencilViewTag>;
using RHI_ShaderResourceView = RHI_Handle<RHI_ShaderResourceViewTag>;
using RHI_UnorderedAccessView = RHI_Handle<RHI_UnorderedAccessViewTag>;

// Синхронизация
using RHI_QueryHandle = RHI_Handle<RHI_QueryTag>;
using RHI_FenceHandle = RHI_Handle<RHI_FenceTag>;

namespace std
{
	template <typename Tag>
	struct hash<RHI_Handle<Tag>>
	{
		size_t operator()(const RHI_Handle<Tag>& h) const noexcept
		{
			return std::hash<u32>{}(h.id);
		}
	};
}
////////////////////////////////////////////////////////////////////////////////
