////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
////////////////////////////////////////////////////////////////////////////////
class SoftXOcclusionCore
{
public:
    SoftXOcclusionCore() = default;
    ~SoftXOcclusionCore();

    // Инициализация / деинициализация
    void Initialize(uint depthMapSize);
    void Shutdown();

    // Доступ к устройству (для создания буферов, запросов и т.д.)
    SoftX::Device& GetDevice() { return *m_device; }
    SoftX::DeviceContext& GetImmediateContext() { return m_device->GetImmediateContext(); }

    // Двойная буферизация
    SoftX::DepthBuffer* GetWriteBuffer() { return m_depthBuffers[m_writeIdx].get(); }
    SoftX::DepthBuffer* GetReadBuffer() { return m_depthBuffers[m_readIdx].get(); }
    void SwapBuffers();
    void WaitForBuildAndSwap(); // ждёт завершения асинхронной задачи и делает свап буфферов

    // Запускает переданную задачу асинхронно. Ожидается, что задача будет
    // заполнять write-буфер (GetWriteBuffer()) через GetImmediateContext().
    void StartBuildTask(std::function<void()> task);

    bool IsDeviceValid() const { return m_device != nullptr; }

private:
    std::unique_ptr<SoftX::Device> m_device;
    std::unique_ptr<SoftX::DepthBuffer> m_depthBuffers[2];
    int m_writeIdx = 0;
    int m_readIdx = 1;
    std::future<void> m_buildFuture;
};
////////////////////////////////////////////////////////////////////////////////
