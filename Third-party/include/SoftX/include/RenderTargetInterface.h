/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "ThirdPartyIncluding.h"
#include "LibInternal.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API IRenderTarget
{
  public:
	virtual ~IRenderTarget() = default;

	virtual void Clear(const float4& color) = 0;

	virtual void SetPixel(uint2 coords, const float4& color) = 0;

	virtual uint Width() const = 0;
	virtual uint Height() const = 0;
	virtual uint2 Size() const = 0;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
