///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_detail_object : public IBlender
{
  public:
	xrP_BOOL oBlend;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: detail objects";
	}

	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	CBlender_detail_object()
	{
		description.CLS = B_DETAIL_OBJECT;
		description.version = 0;
	}

	~CBlender_detail_object() = default;

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
		xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
	}

	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
		xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_DETAIL_NORMAL_ANIMATED:
			C.set_Define("USE_DETAILWAVE", "1", CBlender_Compile::ShaderScope::Vertex);
			configure_shader_detail_object(C, false, "detail_object", "detail_object");
			break;
		case SE_DETAIL_NORMAL_STATIC:
			configure_shader_detail_object(C, false, "detail_object", "detail_object");
			break;
		case SE_DETAIL_SHADOW_DEPTH_ANIMATED:
			C.set_Define("USE_DETAILWAVE", "1", CBlender_Compile::ShaderScope::Vertex);
			C.begin_Pass("shadow_depth_stage_detail_object", "shadow_depth_stage_detail_object", "main", "main", FALSE, TRUE, TRUE);
			C.set_Sampler("s_base", C.L_textures[0]);
			jitter(C);
			C.end_Pass();
			break;
		case SE_DETAIL_SHADOW_DEPTH_STATIC:
			C.begin_Pass("shadow_depth_stage_detail_object", "shadow_depth_stage_detail_object", "main", "main", FALSE, TRUE, TRUE);
			C.set_Sampler("s_base", C.L_textures[0]);
			jitter(C);
			C.end_Pass();
			break;
			// case SE_DETAIL_DEPTH_PREPASS_ANIMATED:
			//	C.set_Define("USE_DETAILWAVE", "1");
			// case SE_DETAIL_DEPTH_PREPASS_STATIC:
			//	C.begin_Pass("shadow_depth_stage_detail_object", "shadow_depth_stage_detail_object",
			//FALSE);//C.begin_Pass("depth_prepass_stage_detail_object", "depth_prepass_stage_detail_object");
			//	C.set_Sampler("s_base", C.L_textures[0]);
			//	jitter(C);
			//	C.end_Pass();
			//	break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
