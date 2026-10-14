#ifndef TSS_H
#define TSS_H
#pragma once

#include "tss_def.h"

class ENGINE_API CSimulatorRS
{
  public:
	IC void Set(SimulatorStates& container, u32 N, u32 V)
	{
		R_ASSERT(N < 256);
		container.set_RS(N, V);
	}
};

class ENGINE_API CSimulator
{
  public:
	CSimulatorRS RS;
	SimulatorStates container;

  public:
	CSimulator()
	{
		Invalidate();
	}
	IC void Invalidate()
	{
		container.clear();
	}
	IC void SetSAMP(u32 S, u32 N, u32 V)
	{
		container.set_SAMP(S, N, V);
	}
	IC void SetRS(u32 N, u32 V)
	{
		RS.Set(container, N, V);
	}
	IC SimulatorStates& GetContainer()
	{
		return container;
	}
};

#endif
