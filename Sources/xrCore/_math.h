#pragma once

#include "math_types.h"
#include "cpuid.h"

namespace CPU
{
XRCORE_API extern u64 clk_per_second;
XRCORE_API extern u64 clk_per_milisec;
XRCORE_API extern u64 clk_per_microsec;
XRCORE_API extern u64 clk_overhead;
XRCORE_API extern float clk_to_seconds;
XRCORE_API extern float clk_to_milisec;
XRCORE_API extern float clk_to_microsec;

XRCORE_API extern u64 qpc_freq;
XRCORE_API extern u64 qpc_overhead;
XRCORE_API extern u32 qpc_counter;

XRCORE_API extern processor_info ID;

XRCORE_API u64 QPC();
XRCORE_API u64 GetCLK(); // Просто объявление

void Detect();
}; // namespace CPU

// Глобальные функции инициализации
XRCORE_API void _initialize_cpu();
XRCORE_API void _initialize_cpu_thread();

// Работа с потоками
typedef void thread_t(void*);
XRCORE_API void thread_name(const char* name);
