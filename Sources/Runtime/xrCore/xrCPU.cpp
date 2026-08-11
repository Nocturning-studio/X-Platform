#include "stdafx.h"
#include "xrCPU.h"
#include <intrin.h>
#include <thread>

namespace CPU
{
XRCORE_API u64 clk_per_second = 0;
XRCORE_API u64 clk_per_milisec = 0;
XRCORE_API u64 clk_per_microsec = 0;
XRCORE_API u64 clk_overhead = 0;
XRCORE_API float clk_to_seconds = 0.0f;
XRCORE_API float clk_to_milisec = 0.0f;
XRCORE_API float clk_to_microsec = 0.0f;

XRCORE_API u64 qpc_freq = 0;
XRCORE_API u64 qpc_overhead = 0;
XRCORE_API u32 qpc_counter = 0;

XRCORE_API processor_info ID;

XRCORE_API u64 QPC()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	qpc_counter++;
	return static_cast<u64>(li.QuadPart);
}

// Реализация GetCLK через интринсик
XRCORE_API u64 GetCLK()
{
	return __rdtsc();
}

namespace Impl
{
void Detect()
{
	// 1. Частота QPC
	LARGE_INTEGER liFreq;
	if (QueryPerformanceFrequency(&liFreq))
		qpc_freq = static_cast<u64>(liFreq.QuadPart);
	else
		qpc_freq = 1000;

	// 2. Оверхед QPC
	u64 start = QPC();
	for (int i = 0; i < 256; i++)
		QPC();
	u64 end = QPC();
	qpc_overhead = (end - start) / 256;

	// 3. Оверхед RDTSC
	start = GetCLK();
	for (int i = 0; i < 256; i++)
		GetCLK();
	end = GetCLK();
	clk_overhead = (end - start) / 256;

	// 4. Калибровка
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	u64 qpc_start = QPC();
	u64 tsc_start = GetCLK();

	u64 qpc_wait = qpc_freq / 10; // Ждем 100мс

	while ((QPC() - qpc_start) < qpc_wait)
	{
		_mm_pause();
	}

	u64 qpc_end = QPC();
	u64 tsc_end = GetCLK();

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	u64 qpc_elapsed = qpc_end - qpc_start;
	u64 tsc_elapsed = tsc_end - tsc_start;

	if (qpc_elapsed > 0)
		clk_per_second = (tsc_elapsed * qpc_freq) / qpc_elapsed;
	else
		clk_per_second = 2500000000ULL; // Фолбэк

	clk_per_milisec = clk_per_second / 1000;
	clk_per_microsec = clk_per_milisec / 1000;
	clk_to_seconds = 1.0f / (float)clk_per_second;
	clk_to_milisec = 1000.0f / (float)clk_per_second;
	clk_to_microsec = 1000000.0f / (float)clk_per_second;
}
} // namespace Impl

void Initialize()
{
	if (!query_processor_info(&CPU::ID))
		FATAL("! Can't detect CPU info");

	CPU::Impl::Detect();

	Msg("* CPU Info:");
	Msg("* CPU Frequency: ~%.2f MHz", float(CPU::clk_per_second) / 1000000.0f);
	Msg("* CPU Hardware Threads: %d", std::thread::hardware_concurrency());

	string256 features;
	strcpy_s(features, "RDTSC");
	if (CPU::ID.hasFeature(CpuFeature::Sse))
		strcat_s(features, ", SSE");
	if (CPU::ID.hasFeature(CpuFeature::Sse2))
		strcat_s(features, ", SSE2");
	if (CPU::ID.hasFeature(CpuFeature::Sse3))
		strcat_s(features, ", SSE3");
	if (CPU::ID.hasFeature(CpuFeature::Sse41))
		strcat_s(features, ", SSE4.1");
	if (CPU::ID.hasFeature(CpuFeature::Sse42))
		strcat_s(features, ", SSE4.2");
	if (CPU::ID.hasFeature(CpuFeature::HT))
		strcat_s(features, ", HTT");

	Msg("* CPU features: %s", features);

	// Включаем Flush-to-Zero (FTZ) и Denormals-are-Zero (DAZ).
	// Это предотвращает падение FPS, когда значения становятся очень близкими к нулю.
	_mm_setcsr(_mm_getcsr() | 0x8000 | 0x0040);

	::Random.seed(u32(CPU::GetCLK()));
}

} // namespace CPU
