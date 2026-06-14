#include "pch.h"
#include <SoftX/SoftX.h>

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
