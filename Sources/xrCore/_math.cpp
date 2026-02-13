#include "stdafx.h"
#pragma hdrstop

#include <thread>
#include <chrono>
#include <intrin.h> // Для __rdtsc
#include "../xrEngine/optick_include.h"

// Глобальные объекты
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
XRCORE_API CRandom Random;

// Реализация функций внутри пространства имен CPU
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
} // namespace CPU

//------------------------------------------------------------------------------------
void _initialize_cpu_thread()
{
	// Включаем Flush-to-Zero и Denormals-are-Zero для SSE
	_mm_setcsr(_mm_getcsr() | 0x8000 | 0x0040);
}

//------------------------------------------------------------------------------------
void _initialize_cpu(void)
{
	if (!query_processor_info(&CPU::ID))
		FATAL("! Can't detect CPU info");

	CPU::Detect();

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

	Fidentity.identity();
	Didentity.identity();
	pvInitializeStatics();

	_initialize_cpu_thread();

	// Здесь мы вызываем CPU::GetCLK() явно, используя пространство имен
	::Random.seed(u32(CPU::GetCLK()));
}

//------------------------------------------------------------------------------------
void thread_name(const char* name)
{
	if (!IsDebuggerPresent())
		return;

#pragma pack(push, 8)
	struct THREAD_NAME
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	};
#pragma pack(pop)

	THREAD_NAME tn;
	tn.dwType = 0x1000;
	tn.szName = name;
	tn.dwThreadID = DWORD(-1);
	tn.dwFlags = 0;
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(tn) / sizeof(DWORD), (ULONG_PTR*)&tn);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

// Сплайны (math implementation)
void spline1(float t, Fvector* p, Fvector* ret)
{
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];

	ret->x = 0.0f;
	ret->y = 0.0f;
	ret->z = 0.0f;
	m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
	m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
	m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
	m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

	for (int i = 0; i < 4; i++)
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

void spline2(float t, Fvector* p, Fvector* ret)
{
	float s = 1.0f - t;
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];

	m[0] = s * s * s;
	m[1] = 3.0f * t3 - 6.0f * t2 + 4.0f;
	m[2] = -3.0f * t3 + 3.0f * t2 + 3.0f * t + 1;
	m[3] = t3;

	ret->x = (p[0].x * m[0] + p[1].x * m[1] + p[2].x * m[2] + p[3].x * m[3]) / 6.0f;
	ret->y = (p[0].y * m[0] + p[1].y * m[1] + p[2].y * m[2] + p[3].y * m[3]) / 6.0f;
	ret->z = (p[0].z * m[0] + p[1].z * m[1] + p[2].z * m[2] + p[3].z * m[3]) / 6.0f;
}

#define beta1 1.0f
#define beta2 0.8f

void spline3(float t, Fvector* p, Fvector* ret)
{
	float s = 1.0f - t;
	float t2 = t * t;
	float t3 = t2 * t;
	float b12 = beta1 * beta2;
	float b13 = b12 * beta1;
	float delta = 2.0f - b13 + 4.0f * b12 + 4.0f * beta1 + beta2 + 2.0f;
	float d = 1.0f / delta;
	float b0 = 2.0f * b13 * d * s * s * s;
	float b3 = 2.0f * t3 * d;
	float b1 = d * (2 * b13 * t * (t2 - 3 * t + 3) + 2 * b12 * (t3 - 3 * t2 + 2) + 2 * beta1 * (t3 - 3 * t + 2) +
					beta2 * (2 * t3 - 3 * t2 + 1));
	float b2 = d * (2 * b12 * t2 * (-t + 3) + 2 * beta1 * t * (-t2 + 3) + beta2 * t2 * (-2 * t + 3) + 2 * (-t3 + 1));

	ret->x = p[0].x * b0 + p[1].x * b1 + p[2].x * b2 + p[3].x * b3;
	ret->y = p[0].y * b0 + p[1].y * b1 + p[2].y * b2 + p[3].y * b3;
	ret->z = p[0].z * b0 + p[1].z * b1 + p[2].z * b2 + p[3].z * b3;
}
