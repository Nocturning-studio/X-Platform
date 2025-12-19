///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_hi_z.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_hi_z_mip_chain()
{
	OPTICK_EVENT("CRender::create_hi_z_mip_chain");
	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	ref_rt MipChain = RenderTarget->rt_Hi_z;

	// Шаг 1: Копируем глубину сцены в Mip 0
	// Используем индекс 0, который мы настроили в блендере
	RenderBackend.set_Element(RenderTarget->s_hi_z->E[SE_HI_Z_GENERATE_MIP_CHAIN_PASS], 0);
	RenderBackend.RenderViewportSurface(MipChain->get_surface_level(0));

	// Шаг 2: Генерируем остальные мипы
	for (u32 i = 1; i < MipChain->get_levels_count(); i++)
	{
		// Используем индекс 1 для генерации (Downsample pass)
		RenderBackend.set_Element(RenderTarget->s_hi_z->E[SE_HI_Z_GENERATE_MIP_CHAIN_PASS], 1);

		u32 prev_mip = i - 1;
		u32 prev_mip_width, prev_mip_height;

		// Получаем размеры ПРЕДЫДУЩЕГО уровня, из которого будем читать
		MipChain->get_level_desc(prev_mip, prev_mip_width, prev_mip_height);

		// Передаем размеры текселя предыдущего мипа, чтобы точно попасть в центры пикселей
		// x = 0, y = index mip-а для чтения, z/w = размер текселя
		RenderBackend.set_Constant("mip_data", 0.0f, (float)prev_mip, 1.0f / prev_mip_width, 1.0f / prev_mip_height);

		// Рендерим в ТЕКУЩИЙ уровень
		RenderBackend.RenderViewportSurface(MipChain->get_surface_level(i));
	}
}
///////////////////////////////////////////////////////////////////////////////////
