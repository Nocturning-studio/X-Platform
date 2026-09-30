////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026 21:19:14
// Author: NS_Deathman
// File: xrRHI_States.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrRHI_Types.h"
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// RHI_BlendState
// ============================================================================
//
// Агрегированное состояние смешивания. Поля соответствуют D3D9 render states:
//   enable        -> D3DRS_ALPHABLENDENABLE
//   srcColor      -> D3DRS_SRCBLEND
//   dstColor      -> D3DRS_DESTBLEND
//   opColor       -> D3DRS_BLENDOP
//   separateAlpha -> D3DRS_SEPARATEALPHABLENDENABLE
//   srcAlpha      -> D3DRS_SRCBLENDALPHA
//   dstAlpha      -> D3DRS_DESTBLENDALPHA
//   opAlpha       -> D3DRS_BLENDOPALPHA
//   writeMask     -> D3DRS_COLORWRITEENABLE0..3
//
// В D3D12 это разложится в D3D12_BLEND_DESC + RT blend states.
//
// ============================================================================

struct RHI_BlendState
{
	bool enable = false;
	RHI_Blend srcColor = RHI_Blend::One;
	RHI_Blend dstColor = RHI_Blend::Zero;
	RHI_BlendOp opColor = RHI_BlendOp::Add;

	bool separateAlpha = false;
	RHI_Blend srcAlpha = RHI_Blend::One;
	RHI_Blend dstAlpha = RHI_Blend::Zero;
	RHI_BlendOp opAlpha = RHI_BlendOp::Add;

	uint8_t writeMask = 0x0F; // RGBA per-channel

	constexpr bool operator==(const RHI_BlendState& o) const noexcept
	{
		return enable == o.enable &&
			   srcColor == o.srcColor &&
			   dstColor == o.dstColor &&
			   opColor == o.opColor &&
			   separateAlpha == o.separateAlpha &&
			   srcAlpha == o.srcAlpha &&
			   dstAlpha == o.dstAlpha &&
			   opAlpha == o.opAlpha &&
			   writeMask == o.writeMask;
	}
	constexpr bool operator!=(const RHI_BlendState& o) const noexcept { return !(*this == o); }

	// ---- Presets ----

	static RHI_BlendState Opaque()
	{
		RHI_BlendState s;
		s.enable = false;
		return s;
	}

	static RHI_BlendState AlphaBlend()
	{
		RHI_BlendState s;
		s.enable = true;
		s.srcColor = RHI_Blend::SrcAlpha;
		s.dstColor = RHI_Blend::InvSrcAlpha;
		s.opColor = RHI_BlendOp::Add;
		return s;
	}

	static RHI_BlendState Additive()
	{
		RHI_BlendState s;
		s.enable = true;
		s.srcColor = RHI_Blend::SrcAlpha;
		s.dstColor = RHI_Blend::One;
		s.opColor = RHI_BlendOp::Add;
		return s;
	}

	static RHI_BlendState Multiply()
	{
		RHI_BlendState s;
		s.enable = true;
		s.srcColor = RHI_Blend::DestColor;
		s.dstColor = RHI_Blend::Zero;
		return s;
	}

	static RHI_BlendState NoColorWrite()
	{
		RHI_BlendState s;
		s.writeMask = 0;
		return s;
	}
};

// ============================================================================
// RHI_DepthStencilState
// ============================================================================
//
// Соответствие D3D9 render states:
//   depthEnable         -> D3DRS_ZENABLE
//   depthWriteEnable    -> D3DRS_ZWRITEENABLE
//   depthFunc           -> D3DRS_ZFUNC
//   stencilEnable       -> D3DRS_STENCILENABLE
//   stencilFunc         -> D3DRS_STENCILFUNC
//   stencilReadMask     -> D3DRS_STENCILMASK
//   stencilWriteMask    -> D3DRS_STENCILWRITEMASK
//   stencilRef          -> D3DRS_STENCILREF
//   stencilFailOp       -> D3DRS_STENCILFAIL
//   stencilDepthFailOp  -> D3DRS_STENCILZFAIL
//   stencilPassOp       -> D3DRS_STENCILPASS
//
// ============================================================================

struct RHI_DepthStencilState
{
	bool depthEnable = true;
	bool depthWriteEnable = true;
	RHI_CmpFunc depthFunc = RHI_CmpFunc::LessEqual;

	bool stencilEnable = false;
	RHI_CmpFunc stencilFunc = RHI_CmpFunc::Always;
	uint8_t stencilReadMask = 0xFF;
	uint8_t stencilWriteMask = 0xFF;
	uint8_t stencilRef = 0;
	RHI_StencilOp stencilFailOp = RHI_StencilOp::Keep;
	RHI_StencilOp stencilDepthFailOp = RHI_StencilOp::Keep;
	RHI_StencilOp stencilPassOp = RHI_StencilOp::Keep;

	constexpr bool operator==(const RHI_DepthStencilState& o) const noexcept
	{
		if(depthEnable != o.depthEnable)
			return false;
		if(depthWriteEnable != o.depthWriteEnable)
			return false;
		if(depthFunc != o.depthFunc)
			return false;
		if(stencilEnable != o.stencilEnable)
			return false;
		if(!stencilEnable)
			return true;
		return stencilFunc == o.stencilFunc &&
			   stencilReadMask == o.stencilReadMask &&
			   stencilWriteMask == o.stencilWriteMask &&
			   stencilRef == o.stencilRef &&
			   stencilFailOp == o.stencilFailOp &&
			   stencilDepthFailOp == o.stencilDepthFailOp &&
			   stencilPassOp == o.stencilPassOp;
	}
	constexpr bool operator!=(const RHI_DepthStencilState& o) const noexcept { return !(*this == o); }

	// ---- Presets ----

	static RHI_DepthStencilState Default()
	{
		return {};
	}

	static RHI_DepthStencilState DepthReadOnly()
	{
		RHI_DepthStencilState s;
		s.depthWriteEnable = false;
		return s;
	}

	static RHI_DepthStencilState NoDepth()
	{
		RHI_DepthStencilState s;
		s.depthEnable = false;
		s.depthWriteEnable = false;
		return s;
	}
};

// ============================================================================
// RHI_RasterizerState
// ============================================================================
//
// Соответствие D3D9 render states:
//   fillMode             -> D3DRS_FILLMODE
//   cullMode             -> D3DRS_CULLMODE
//   scissorEnable        -> D3DRS_SCISSORTESTENABLE
//   depthBias            -> D3DRS_DEPTHBIAS            (float bits)
//   slopeScaledDepthBias -> D3DRS_SLOPESCALEDEPTHBIAS  (float bits)
//
// frontCounterClockwise в D3D9 не управляется отдельным render state — там
// фиксированное правило обхода (CW). Оставляем поле для совместимости с D3D12.
//
// ============================================================================

struct RHI_RasterizerState
{
	RHI_FillMode fillMode = RHI_FillMode::Solid;
	RHI_CullMode cullMode = RHI_CullMode::CounterClockwise;
	bool frontCounterClockwise = false;

	float depthBias = 0.0f;
	float slopeScaledDepthBias = 0.0f;

	bool scissorEnable = false;

	constexpr bool operator==(const RHI_RasterizerState& o) const noexcept
	{
		return fillMode == o.fillMode &&
			   cullMode == o.cullMode &&
			   frontCounterClockwise == o.frontCounterClockwise &&
			   depthBias == o.depthBias &&
			   slopeScaledDepthBias == o.slopeScaledDepthBias &&
			   scissorEnable == o.scissorEnable;
	}
	constexpr bool operator!=(const RHI_RasterizerState& o) const noexcept { return !(*this == o); }

	// ---- Presets ----

	static RHI_RasterizerState Default() { return {}; }

	static RHI_RasterizerState CullNone()
	{
		RHI_RasterizerState s;
		s.cullMode = RHI_CullMode::None;
		return s;
	}

	static RHI_RasterizerState CullBack()
	{
		RHI_RasterizerState s;
		s.cullMode = RHI_CullMode::CounterClockwise;
		return s;
	}

	static RHI_RasterizerState Wireframe()
	{
		RHI_RasterizerState s;
		s.fillMode = RHI_FillMode::Wireframe;
		return s;
	}
};
////////////////////////////////////////////////////////////////////////////////
