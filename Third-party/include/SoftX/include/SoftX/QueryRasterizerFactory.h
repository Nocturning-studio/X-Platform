#pragma once

#include "LibInternal.h"
#include "QueryRasterizerInterface.h"

SOFTX_BEGIN

std::unique_ptr<IQueryRasterizer> CreateBestQueryRasterizer();

SOFTX_END
