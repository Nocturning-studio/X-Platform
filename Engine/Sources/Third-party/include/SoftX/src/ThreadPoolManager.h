/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/LibInternal.h"
#include "../include/Exceptions.h"
#include "ThreadPool.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class ThreadPoolManager 
{
public:
    static void Initialize(uint32_t threadCount) 
    {
        if (instance) SOFTX_THROW(InvalidState("ThreadPoolManager already initialized"));
        instance = std::make_unique<ThreadPool>(threadCount);
    }

    static ThreadPool& Get() 
    {
        if (!instance) SOFTX_THROW(InvalidState("ThreadPoolManager not initialized. Call Device constructor first."));
        return *instance;
    }

    static void Shutdown()
    {
        instance.reset();
    }

private:
    static inline std::unique_ptr<ThreadPool> instance = nullptr;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
