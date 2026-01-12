#include "stdafx.h"
#pragma hdrstop

#include "r_constants_cache.h"

// Кастыль для исправления ошибки зависимости от Device!!!
u32 R_constant_array::getFrame()
{
	return Engine.TimeManager.GetFrameCount();
}

void R_constants::flush_cache()
{
	// ВРЕМЕННО: ДЕБАГ ВЫВОД
#ifdef DEBUG
	static u32 lastDebugFrame = 0;
	if (Engine.TimeManager.GetFrameCount() - lastDebugFrame > 60) // Раз в секунду
	{
		Msg("* CONSTANTS DEBUG: Frame %d, Pixel dirty: %d (needs_flush: %d), Vertex dirty: %d (needs_flush: %d)",
			Engine.TimeManager.GetFrameCount(), a_pixel.b_dirty, a_pixel.needs_flush(), a_vertex.b_dirty, a_vertex.needs_flush());
		lastDebugFrame = Engine.TimeManager.GetFrameCount();
	}
#endif

	// ВРЕМЕННО: используем старую логику для стабильности
	if (a_pixel.b_dirty)
	{
		R_constant_array::t_f& F = a_pixel.c_f;
		{
			if (F.r_lo() <= 32)
			{
				u32 count = F.r_hi() - F.r_lo();
				if (count)
				{
					count = (count > 31) ? 31 : count;
					CHK_DX(HW.pDevice->SetPixelShaderConstantF(F.r_lo(), (float*)F.access(F.r_lo()), count));
					F.flush();
				}
			}
		}
		a_pixel.b_dirty = FALSE;

		// ТЕСТИРУЕМ новую систему параллельно
		a_pixel.mark_flushed();
	}

	if (a_vertex.b_dirty)
	{
		R_constant_array::t_f& F = a_vertex.c_f;
		{
			u32 count = F.r_hi() - F.r_lo();
			if (count)
			{
#ifdef DEBUG
				if (F.r_hi() > HW.Caps.geometry.dwRegisters)
				{
					Debug.fatal(DEBUG_INFO, "Internal error setting VS-constants: overflow\nregs[%d],hi[%d]",
								HW.Caps.geometry.dwRegisters, F.r_hi());
				}
#endif
				CHK_DX(HW.pDevice->SetVertexShaderConstantF(F.r_lo(), (float*)F.access(F.r_lo()), count));
				F.flush();
			}
		}
		a_vertex.b_dirty = FALSE;

		// ТЕСТИРУЕМ новую систему параллельно
		a_vertex.mark_flushed();
	}

	// ДОПОЛНИТЕЛЬНО: проверяем работу новой системы
	if (a_pixel.needs_flush() || a_vertex.needs_flush())
	{
		Msg("! CONSTANTS WARNING: New dirty system detected unflushed constants!");
	}
}