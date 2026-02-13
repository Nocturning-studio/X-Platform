#pragma once

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

namespace Impl
{
void Detect();
}

XRCORE_API u64 QPC();
XRCORE_API u64 GetCLK();
XRCORE_API void Initialize();
}; // namespace CPU

