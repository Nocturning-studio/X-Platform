#include "pch.h"
#include "xrRHI.h"
#include "Backends/DirectX9/xrBackendDX9.h"

extern "C"
{
	XRRHI_API IRenderBackend* CreateRenderBackend(RHI_BackendType type)
	{
		switch(type)
		{
		case RHI_BackendType::DirectX9:
			return new CRenderBackendDX9();
		default:
			return nullptr;
		}
	}
}
