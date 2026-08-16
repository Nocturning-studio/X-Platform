/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "ThreadPoolManager.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

namespace ThreadUtils
{

inline void DispatchWorkers(const std::function<void()>& task)
{
    PROFILE_SCOPE("DispatchWorkers");
    auto& pool = ThreadPoolManager::Get();
    const size_t n = pool.threadCount();
    for (size_t i = 0; i < n; ++i)
        pool.enqueue(task);
    pool.wait();
}

template <typename Index, typename Func>
void SmartParallelFor(Index start, Index end, Index step, Func&& func)
{
    if (end <= start) return;

    auto& pool = ThreadPoolManager::Get();
    const uint32_t numThreads = pool.threadCount();
    const Index total = (end - start + step - 1) / step;

    if (numThreads <= 1 || total < 64)
    {
        for (Index i = start; i < end; i += step)
            func(i);
        return;
    }

    std::atomic<Index> next(start);

    DispatchWorkers([&]()
        {
            while (true)
            {
                Index i = next.fetch_add(step);
                if (i >= end)
                    break;

                func(i);
            }
        });
}

} // namespace ThreadUtils

SOFTX_END
/////////////////////////////////////////////////////////////////
