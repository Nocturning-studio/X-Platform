////////////////////////////////////////////////////////////////////////////////
// Created: 02.10.2026 19:43:31
// Author: NS_Deathman
// File: RHI.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "RHI.h"
////////////////////////////////////////////////////////////////////////////////
CXRHISubsystem::~CXRHISubsystem()
{
	Destroy();
}

bool CXRHISubsystem::Initialize(RHI_BackendType type)
{
	if (m_backend)
		return true;

	m_type = type;
	m_backend = new CRenderBackend();

	Msg("* [RHI] Subsystem initialized (backend type = %u)", (u32)type);
	return true;
}

void CXRHISubsystem::Destroy()
{
	if (m_backend)
	{
		m_backend->DestroyDevice();
		delete m_backend;
		m_backend = nullptr;
	}
	Msg("* [RHI] Subsystem shut down");
}

bool CXRHISubsystem::CreateDevice(HWND hWnd, const RHI_PresentationParams& params)
{
	if (!m_backend) return false;
	if (m_backend->IsReady())
	{
		Msg("! [RHI] CreateDevice: device already created");
		return false;
	}

	if (!m_backend->CreateDevice(hWnd, m_type, params))
		return false;

	return true;
}

void CXRHISubsystem::DestroyDevice()
{
	if (m_backend) m_backend->DestroyDevice();
}

bool CXRHISubsystem::ResetDevice(const RHI_PresentationParams& params)
{
	if (!m_backend) return false;
	return m_backend->ResetDevice(params);
}

void CXRHISubsystem::BeginFrame()
{
	if (m_backend) m_backend->BeginFrame();
}

void CXRHISubsystem::EndFrame()
{
	if (m_backend) m_backend->EndFrame();
}

void CXRHISubsystem::Present()
{
	if (m_backend) m_backend->Present();
}

bool CXRHISubsystem::NeedReset() const
{
	if (!m_backend || !m_backend->IsReady())
		return false;
	return m_backend->NeedReset();
}

RHI_DeviceStatus CXRHISubsystem::CheckStatus() const
{
	if (!m_backend || !m_backend->IsReady())
		return RHI_DeviceStatus::Lost;
	return m_backend->GetRHI()->CheckDeviceStatus();
}

IRenderBackend* CXRHISubsystem::GetRawRHI() const
{
	if (!m_backend) return nullptr;
	return m_backend->GetRHI();
}

u32 CXRHISubsystem::GetBackBufferWidth() const
{
	if (!m_backend) return 0;
	return m_backend->GetBackBufferWidth();
}

u32 CXRHISubsystem::GetBackBufferHeight() const
{
	if (!m_backend) return 0;
	return m_backend->GetBackBufferHeight();
}
////////////////////////////////////////////////////////////////////////////////
