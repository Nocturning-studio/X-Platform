/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"
#include "../include/SoftX.h"
#include "../include/RasterizerInterface.h"
#include "RasterizerAVX.h"
#include "RasterizerSSE.h"
#include "RasterizerScalar.h"
#include "CPUDetector.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

std::unique_ptr<IRasterizer> CreateBestRasterizer()
{
	const auto& caps = CPUDetector::GetCapabilities();

	if (caps.avx)
		return std::make_unique<RasterizerAVX>();
	else if (caps.sse41)
		return std::make_unique<RasterizerSSE>();
	else
		return std::make_unique<RasterizerScalar>();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
