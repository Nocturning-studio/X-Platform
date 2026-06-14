#include "pch.h"
#include <SoftX/SoftX.h>
#include <array>
#include <cstring>

#ifdef _WIN32
#include <intrin.h>
#endif

SOFTX_BEGIN

CPUCapabilities CPUDetector::s_caps{};
bool CPUDetector::s_initialized = false;

void CPUDetector::Initialize()
{
	if (s_initialized)
		return;

	std::memset(&s_caps, 0, sizeof(s_caps));

#if defined(_MSC_VER) && defined(_WIN32)
	std::array<int, 4> cpui;
	__cpuidex(cpui.data(), 0, 0);
	int nIds = cpui[0];

	std::vector<std::array<int, 4>> data;
	for (int i = 0; i <= nIds; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		data.push_back(cpui);
	}

	if (nIds >= 1)
	{
		const auto& info = data[1];
		s_caps.sse = (info[3] & (1 << 25)) != 0;   // EDX bit 25
		s_caps.sse2 = (info[3] & (1 << 26)) != 0;  // EDX bit 26
		s_caps.sse3 = (info[2] & (1 << 0)) != 0;   // ECX bit 0
		s_caps.ssse3 = (info[2] & (1 << 9)) != 0;  // ECX bit 9
		s_caps.sse41 = (info[2] & (1 << 19)) != 0; // ECX bit 19
		s_caps.sse42 = (info[2] & (1 << 20)) != 0; // ECX bit 20
		s_caps.avx = (info[2] & (1 << 28)) != 0;   // ECX bit 28
	}

	if (nIds >= 7)
	{
		std::array<int, 4> ext;
		__cpuidex(ext.data(), 7, 0);
		s_caps.avx2 = (ext[1] & (1 << 5)) != 0; // EBX bit 5
		s_caps.fma = (ext[1] & (1 << 12)) != 0; // EBX bit 12 (FMA3)
	}

#elif defined(__GNUC__) || defined(__clang__)
	// GCC/Clang: using <cpuid.h>
	unsigned int eax, ebx, ecx, edx;

	__cpuid(0, eax, ebx, ecx, edx);
	unsigned int maxLevel = eax;

	if (maxLevel >= 1)
	{
		__cpuid(1, eax, ebx, ecx, edx);
		s_caps.sse = (edx & (1 << 25)) != 0;
		s_caps.sse2 = (edx & (1 << 26)) != 0;
		s_caps.sse3 = (ecx & (1 << 0)) != 0;
		s_caps.ssse3 = (ecx & (1 << 9)) != 0;
		s_caps.sse41 = (ecx & (1 << 19)) != 0;
		s_caps.sse42 = (ecx & (1 << 20)) != 0;
		s_caps.avx = (ecx & (1 << 28)) != 0;
	}

	if (maxLevel >= 7)
	{
		__cpuid_count(7, 0, eax, ebx, ecx, edx);
		s_caps.avx2 = (ebx & (1 << 5)) != 0;
		s_caps.fma = (ebx & (1 << 12)) != 0;
	}

#else
#warning "CPUDetector not implemented for this compiler/OS. Assuming minimal features."
	s_caps.sse = true;
	s_caps.sse2 = true;
	// остальные false
#endif

	s_initialized = true;
}

const CPUCapabilities& CPUDetector::GetCapabilities() {
    Initialize();
    return s_caps;
}

SOFTX_END
