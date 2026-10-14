////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "SoftXOcclusionCore.h"
////////////////////////////////////////////////////////////////////////////////
SoftXOcclusionCore::~SoftXOcclusionCore()
{
	Shutdown();
}

void SoftXOcclusionCore::Initialize(uint2 depthMapSize)
{
	Shutdown();

	m_depth_resolution = depthMapSize;

	Msg("DepthBuffer resolution: %d, %d", m_depth_resolution.x, m_depth_resolution.y);

	SoftX::PresentParameters params;
	params.BackBufferSize = m_depth_resolution;
	params.hDeviceWindow = nullptr;
	params.Windowed = true;
	params.Headless = true;

	m_device = std::make_unique<SoftX::Device>(params, 4);

	// Создаём два depth-буфера для двойной буферизации
	m_depthBuffers[0] = std::make_unique<SoftX::DepthBuffer>(m_depth_resolution, 4);
	m_depthBuffers[1] = std::make_unique<SoftX::DepthBuffer>(m_depth_resolution, 4);

	// Начальные настройки контекста (общие для всех пользователей)
	SoftX::DeviceContext& ctx = m_device->GetImmediateContext();
	ctx.SetRenderTarget(nullptr, true);
	ctx.SetDepthBuffer(m_depthBuffers[m_writeIdx]);
	ctx.SetCullMode(SoftX::CullMode::None);
	ctx.SetFillMode(SoftX::FillMode::Solid);
	ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
	ctx.SetDepthWriteEnable(true);
	ctx.SetViewport(SoftX::Viewport(0.0f, 0.0f, (float)depthMapSize.x, (float)depthMapSize.y, 0.0f, 1.0f));
	ctx.SetTileSize(64);

	m_readBufferReady = false;
}

void SoftXOcclusionCore::Shutdown()
{
	if(m_buildFuture.valid())
		m_buildFuture.wait();

	m_depthBuffers[0].reset();
	m_depthBuffers[1].reset();
	m_device.reset();
	m_readBufferReady = false;
}

void SoftXOcclusionCore::SwapBuffers()
{
	std::swap(m_writeIdx, m_readIdx);
	m_readBufferReady = true;
}

void SoftXOcclusionCore::WaitForBuildAndSwap()
{
	if(m_buildFuture.valid())
	{
		m_buildFuture.wait();
		SwapBuffers();
		m_buildFuture = {};
	}
}
////////////////////////////////////////////////////////////////////////////////
