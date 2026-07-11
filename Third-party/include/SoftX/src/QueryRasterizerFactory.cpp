/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"
#include "../include/SoftX.h"
#include "../include/QueryRasterizerInterface.h"
#include "QueryRasterizerAVX.h"
#include "QueryRasterizerSSE.h"
#include "QueryRasterizerScalar.h"
#include "CPUDetector.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

std::unique_ptr<IQueryRasterizer> CreateBestQueryRasterizer()
{
	const auto& caps = CPUDetector::GetCapabilities();

	if (caps.avx)
		return std::make_unique<QueryRasterizerAVX>();
	else if (caps.sse41)
		return std::make_unique<QueryRasterizerSSE>();
	else
		return std::make_unique<QueryRasterizerScalar>();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
