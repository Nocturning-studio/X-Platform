#include "pch.h"

#include <SoftX/ThreadPoolManager.h>

SOFTX_BEGIN

std::unique_ptr<ThreadPool> ThreadPoolManager::s_pool;
std::once_flag ThreadPoolManager::s_initFlag;

ThreadPool& ThreadPoolManager::Get()
{
	std::call_once(s_initFlag, []() { s_pool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency()); });
	return *s_pool;
}

SOFTX_END
