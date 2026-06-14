#pragma once

#define SOFTX_API
#define SOFTX_BEGIN namespace SoftX {
#define SOFTX_END }

#if _HAS_CXX20
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#else
#define LIKELY
#define UNLIKELY
#endif
