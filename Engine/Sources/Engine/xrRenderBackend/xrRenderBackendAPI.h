#pragma once

#if defined(XRRENDERBACKEND_EXPORTS)
#define XRRB_API __declspec(dllexport)
#elif defined(ENGINE_STATIC)
#define XRRB_API __declspec(dllexport)
#else
#define XRRB_API __declspec(dllimport)
#endif

#define RELEASE(x)          \
	{                       \
		if(x)               \
		{                   \
			(x)->Release(); \
			(x) = NULL;     \
		}                   \
	}

#define SHOW_REF(msg, x)                      \
	{                                         \
		if(x)                                 \
		{                                     \
			x->AddRef();                      \
			Log(msg, uint32_t(x->Release())); \
		}                                     \
	}
