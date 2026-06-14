#pragma once

#include <memory>
#include "LibInternal.h"
#include "RasterizerInterface.h"

SOFTX_BEGIN

std::unique_ptr<IRasterizer> CreateBestRasterizer();

SOFTX_END
