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
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
#include <future>
////////////////////////////////////////////////////////////////////////////////
class SoftXOcclusionCore
{
  public:
	SoftXOcclusionCore() = default;
	~SoftXOcclusionCore();

	// Инициализация / деинициализация
	void Initialize(uint2 depthMapSize);
	void Shutdown();

	// Доступ к устройству (для создания буферов, запросов и т.д.)
	SoftX::Device& GetDevice() { return *m_device; }
	SoftX::DeviceContext& GetImmediateContext() { return m_device->GetImmediateContext(); }

	// Двойная буферизация
	std::shared_ptr<SoftX::DepthBuffer> GetWriteBuffer() { return m_depthBuffers[m_writeIdx]; }
	std::shared_ptr<SoftX::DepthBuffer> GetReadBuffer() { return m_depthBuffers[m_readIdx]; }
	void SetBuildFuture(std::future<void>&& fut) { m_buildFuture = std::move(fut); }
	void SwapBuffers();
	void WaitForBuildAndSwap();

	bool IsReadBufferReady() const { return m_readBufferReady; }

	bool IsDeviceValid() const { return m_device != nullptr; }

	uint2 GetDepthResolution() const { return m_depth_resolution; }

  private:
	std::unique_ptr<SoftX::Device> m_device;
	std::shared_ptr<SoftX::DepthBuffer> m_depthBuffers[2];
	int m_writeIdx = 0;
	int m_readIdx = 1;
	uint2 m_depth_resolution = 512;
	std::future<void> m_buildFuture;
	bool m_readBufferReady = false;
};
////////////////////////////////////////////////////////////////////////////////
