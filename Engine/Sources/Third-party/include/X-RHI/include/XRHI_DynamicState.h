////////////////////////////////////////////////////////////////////////////////
// Created: 17.10.2026
// Author: NS_Deathman
// File: XRHI_DynamicState.h
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "XRHI_Types.h"
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class RHI_CachedValue
{
public:
	bool NeedsUpdate(const T& v) const
	{
		return !m_valid || !(m_value == v);
	}

	void MarkUpdated(const T& v)
	{
		m_value = v;
		m_valid = true;
	}

	void Invalidate() { m_valid = false; }

	const T& Get() const { return m_value; }
	bool IsValid() const { return m_valid; }

private:
	T m_value{};
	bool m_valid = false;
};

class RHI_DynamicStateCache
{
public:
	void Invalidate()
	{
		m_viewport.Invalidate();
		m_scissor.Invalidate();
		m_stencilRef.Invalidate();
		m_blendFactor.Invalidate();
	}

	void SetRenderTargetSize(uint32_t w, uint32_t h)
	{
		m_rtWidth = w;
		m_rtHeight = h;
	}

	uint32_t GetRenderTargetWidth()  const { return m_rtWidth; }
	uint32_t GetRenderTargetHeight() const { return m_rtHeight; }

	RHI_CachedValue<RHI_Viewport> m_viewport;
	RHI_CachedValue<RHI_Rect> m_scissor;
	RHI_CachedValue<uint8_t> m_stencilRef;
	RHI_CachedValue<Float4> m_blendFactor;

private:
	uint32_t m_rtWidth = 0;
	uint32_t m_rtHeight = 0;
};
////////////////////////////////////////////////////////////////////////////////
