// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable : 4995)
#include "..\xrEngine\stdafx.h"
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)
#pragma warning(disable : 4714)
#pragma warning(4 : 4018)
#pragma warning(4 : 4244)
#pragma warning(disable : 4237)

#include "..\xrEngine\resourcemanager.h"
#include "..\xrEngine\vis_common.h"
#include "..\xrEngine\render.h"
#include "..\xrEngine\igame_level.h"
#include "..\xrEngine\blender.h"
#include "..\xrEngine\blender_clsid.h"
#include "..\xrEngine\psystem.h"
#include "xrRender_console.h"
#include "render.h"
#include "r_color_converting.h"
#include "../xrEngine/Engine.h"

IC void jitter(CBlender_Compile& C)
{
	C.set_Sampler("s_blue_noise", "noise\\blue_noise_texture", true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE,
				  D3DTEXF_POINT);
	C.set_Sampler("s_perlin_noise", "noise\\perlin_noise_texture", true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE,
				  D3DTEXF_POINT);
	C.set_Sampler("s_wave_map", "vfx\\vfx_wave_map");
	C.set_Sampler("s_cell_bomber", "vfx\\vfx_cell_bomber");
}

IC void gbuffer(CBlender_Compile& C)
{
	C.set_Sampler_point("s_gbuffer_1", r_RT_GBuffer_1);
	C.set_Sampler_point("s_gbuffer_2", r_RT_GBuffer_2);
	C.set_Sampler_point("s_gbuffer_3", r_RT_GBuffer_3);
	C.set_Sampler_point("s_gbuffer_4", r_RT_GBuffer_4);
}
