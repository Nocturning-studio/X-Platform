#pragma once

#define RHI_BEGIN                                                                                                      \
	namespace xrRHI                                                                                                    \
	{                                                                                                                  \

#define RHI_END }

#ifdef XRRHI_EXPORTS
#define XRRHI_API __declspec(dllexport)
#else
#define XRRHI_API __declspec(dllimport)
#endif

#pragma warning(default : 4996)

#define DEPRECATED

/*
#if defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated("This function/field is deprecated. Use new backend instead."))
#elif defined(__GNUC__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated("This function/field is deprecated. Use new backend instead.")))
#else
#define DEPRECATED
#endif
*/
