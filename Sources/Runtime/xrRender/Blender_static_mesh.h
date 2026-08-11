///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_static_mesh : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: static mesh";
	}

	virtual BOOL canBeDetailed()
	{
		return TRUE;
	}

	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	CBlender_static_mesh()
	{
		description.CLS = B_STATIC_MESH;
	}

	~CBlender_static_mesh() = default;

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
	}
	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_NORMAL_HQ: // deffer
			configure_shader(C, true, "static_mesh", "static_mesh", false);
			break;
		case SE_NORMAL_LQ: // deffer
			configure_shader(C, false, "static_mesh", "static_mesh", false);
			break;
		//case SE_SHADOW_DEPTH: // smap-direct
		//	C.begin_Pass("shadow_depth_stage_static_mesh", "shadow_depth_stage_static_mesh", FALSE, TRUE, TRUE, FALSE);
		//	C.set_Sampler("s_base", C.L_textures[0]);
		//	C.end_Pass();
		//	break;
		//case SE_DEPTH_PREPASS:
		//	C.begin_Pass("depth_prepass_stage_static_mesh", "depth_prepass_stage_static_mesh", FALSE, TRUE, TRUE, FALSE);
		//	C.set_Sampler("s_base", C.L_textures[0]);
		//	C.end_Pass();
		//	break;
		case SE_SHADOW_DEPTH: // smap
		case SE_DEPTH_PREPASS:
			configure_shader(C, true, "static_mesh", "static_mesh", true, true);
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
