#pragma once

#include <memory>

#include "LibInternal.h"
#include "ThreadPool.h"

SOFTX_BEGIN

class SOFTX_API ThreadPoolManager
{
  public:
	static ThreadPool& Get();

  private:
	static std::unique_ptr<ThreadPool> s_pool;
	static std::once_flag s_initFlag;
};

SOFTX_END
