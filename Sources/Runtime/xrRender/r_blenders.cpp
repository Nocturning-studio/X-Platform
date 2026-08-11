#include "stdafx.h"

#include "shader_configurator.h"

#include "Blender_terrain.h"
#include "Blender_static_mesh.h"
#include "Blender_skinned_mesh.h"
#include "Blender_transluent.h"
#include "Blender_multiple_usage.h"
#include "Blender_detail_object.h"
#include "blender_particle.h"
#include "Blender_Model_EbB.h"
#include "blender_Lm(EbB).h"
#include "Blender_Screen_SET.h"

IBlender* CRender::blender_create(CLASS_ID cls)
{
	switch (cls)
	{
	case B_STATIC_MESH:
		return xr_new<CBlender_static_mesh>();
	case B_STATIC_MESH_TRANSLUENT:
		return xr_new<CBlender_transluent>(true);
	case B_VERT:
		return xr_new<CBlender_static_mesh>();
	case B_VERT_AREF:
		return xr_new<CBlender_transluent>(false);
	case B_SCREEN_SET:
		return xr_new<CBlender_Screen_SET>(); // Фиксед пайплайн поебень для шрифтов блядь
	case B_SCREEN_GRAY:
		return 0;
	case B_EDITOR_WIRE:
		return 0;
	case B_EDITOR_SEL:
		return 0;
	case B_LIGHT:
		return 0;
	case B_TERRAIN_LIGHTMAPPED:
		return xr_new<CBlender_terrain>();
	case B_LaEmB:
		return 0;
	case B_LmEbB:
		return xr_new<CBlender_LmEbB>();
	case B_B:
		return 0;
	case B_TERRAIN:
		return xr_new<CBlender_terrain>();
	case B_SHADOW_TEX:
		return 0;
	case B_SHADOW_WORLD:
		return 0;
	case B_BLUR:
		return 0;
	case B_SKINNED_MESH:
		return xr_new<CBlender_skinned_mesh>();
	case B_MODEL_EbB:
		return xr_new<CBlender_Model_EbB>();
	case B_DETAIL_OBJECT:
		return xr_new<CBlender_detail_object>();
	case B_MULTIPLE_USAGE:
		return xr_new<CBlender_multiple_usage>();
	case B_PARTICLE:
		return xr_new<CBlender_Particle>();
	}
	return 0;
}

void CRender::blender_destroy(IBlender*& B)
{
	xr_delete(B);
}
