#include "pch.h"
#include "xrRHI.h"
#include "Backends/DirectX9/xrBackendDX9.h"

extern "C"
{
	XRRHI_API xrRHI::IRenderBackend* CreateRenderBackend(xrRHI::BackendType type)
	{
		switch (type)
		{
		case xrRHI::BackendType::DirectX9:
			return new xrRHI::CRenderBackendDX9();
		default:
			return nullptr;
		}
	}
}
