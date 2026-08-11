////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
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
    void SwapBuffers();
    void WaitForBuildAndSwap(); // ждёт завершения асинхронной задачи и делает свап буфферов

    bool IsReadBufferReady() const { return m_readBufferReady; }

    // Запускает переданную задачу асинхронно. Ожидается, что задача будет
    // заполнять write-буфер (GetWriteBuffer()) через GetImmediateContext().
    void StartBuildTask(std::function<void()> task);

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
