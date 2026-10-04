////////////////////////////////////////////////////////////////////////////////
// Created: 02.10.2026 19:22:25
// Author: NS_Deathman
// File: RHI.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/RenderBackend.h>
#include <xrEngine/xrEngineAPI.h>
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CXRHISubsystem
{
public:
	CXRHISubsystem() = default;
	~CXRHISubsystem();

	CXRHISubsystem(const CXRHISubsystem&) = delete;
	CXRHISubsystem& operator=(const CXRHISubsystem&) = delete;

	bool Initialize(RHI_BackendType type = RHI_BackendType::DirectX9Ex);
	void Destroy();

	bool CreateDevice(HWND hWnd, const RHI_PresentationParams& params);
	void DestroyDevice();
	bool ResetDevice(const RHI_PresentationParams& params);
	bool IsReady() const { return m_backend != nullptr; }

	void BeginFrame();
	void EndFrame();
	void Present();

	bool NeedReset() const;
	RHI_DeviceStatus CheckStatus() const;

	CRenderBackend* Get() const { return m_backend; }
	CRenderBackend* operator->() const { return m_backend; }
	CRenderBackend& operator*() const { return *m_backend; }

	IRenderBackend* GetRawRHI() const;

	u32 GetBackBufferWidth() const;
	u32 GetBackBufferHeight() const;

private:
	RHI_BackendType m_type = RHI_BackendType::DirectX9Ex;
	CRenderBackend* m_backend = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
