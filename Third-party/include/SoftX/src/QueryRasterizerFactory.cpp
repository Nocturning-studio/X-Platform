#include "pch.h"
#include <SoftX/SoftX.h>

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
