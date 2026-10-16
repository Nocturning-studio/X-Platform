////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <include/XRHI.h>
#include "Backends/DirectX9/BackendDX9.h"
// #include "Backends/DirectX12/BackendDX12.h"
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
	XRHI_API IRenderBackend* CreateRenderBackend(RHI_BackendType type)
	{
		switch(type)
		{
		case RHI_BackendType::DirectX9Ex:
			return new CRenderBackendDX9();
		// case RHI_BackendType::DirectX12:
		//	return new CRenderBackendDX12();
		default:
			return nullptr;
		}
	}
}

CRenderBackendDX9* AsDX9(IRenderBackend* b)
{
	if(!b || b->GetBackendType() != RHI_BackendType::DirectX9Ex)
		return nullptr;
	return static_cast<CRenderBackendDX9*>(b);
}

/*
CRenderBackendDX12* AsDX12(IRenderBackend* b)
{
	if (!b || b->GetBackendType() != RHI_BackendType::DirectX12)
		return nullptr;
	return static_cast<CRenderBackendDX12*>(b);
}
*/
////////////////////////////////////////////////////////////////////////////////
