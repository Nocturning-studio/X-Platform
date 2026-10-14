////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include "IRenderStage.h"
////////////////////////////////////////////////////////////////////////////////
class IRenderPipeline
{
public:
    IRenderPipeline() = default;
    virtual ~IRenderPipeline() { };

    virtual std::string_view GetName() const = 0;

    virtual void Initialize() = 0;
    virtual void Destroy() = 0;
    virtual void OnDeviceReset() = 0;

    virtual void OnActivate() {}
    virtual void OnDeactivate() {}

    virtual void Execute();

    void AddStage(std::unique_ptr<IRenderStage> stage);
    void InsertStage(size_t index, std::unique_ptr<IRenderStage> stage);
    bool RemoveStage(std::string_view name);
    IRenderStage* FindStage(std::string_view name) const;

protected:
    std::vector<std::unique_ptr<IRenderStage>> m_stages;
};
////////////////////////////////////////////////////////////////////////////////
