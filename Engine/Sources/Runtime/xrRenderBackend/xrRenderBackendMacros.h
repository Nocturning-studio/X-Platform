#pragma once

#define RELEASE(x)         \
	{                       \
		if(x)               \
		{                   \
			(x)->Release(); \
			(x) = NULL;     \
		}                   \
	}

#define SHOW_REF(msg, x)                \
	{                                    \
		if(x)                            \
		{                                \
			x->AddRef();                 \
			Log(msg, uint32_t(x->Release())); \
		}                                \
	}
