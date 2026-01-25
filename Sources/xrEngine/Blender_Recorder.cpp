// Blender_Recorder.cpp: implementation of the CBlender_Compile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

static int ParseName(LPCSTR N)
{
	if (0 == xr_strcmp(N, "$null"))
		return -1;
	if (0 == xr_strcmp(N, "$base0"))
		return 0;
	if (0 == xr_strcmp(N, "$base1"))
		return 1;
	if (0 == xr_strcmp(N, "$base2"))
		return 2;
	if (0 == xr_strcmp(N, "$base3"))
		return 3;
	if (0 == xr_strcmp(N, "$base4"))
		return 4;
	if (0 == xr_strcmp(N, "$base5"))
		return 5;
	if (0 == xr_strcmp(N, "$base6"))
		return 6;
	if (0 == xr_strcmp(N, "$base7"))
		return 7;
	return -1;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Compile::CBlender_Compile()
{
}
CBlender_Compile::~CBlender_Compile()
{
}
void CBlender_Compile::_cpp_Compile(ShaderElement* _SH)
{
	SH = _SH;
	RS.Invalidate();

	// Analyze possibility to detail this shader
	detail_texture = NULL;
	detail_scaler = NULL;
	LPCSTR base = NULL;
	if (bDetail && BT->canBeDetailed())
	{
		//
		sh_list& lst = L_textures;
		int id = ParseName(BT->oT_Name);
		base = BT->oT_Name;
		if (id >= 0)
		{
			if (id >= int(lst.size()))
				Debug.fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
			base = *lst[id];
		}
		//.		if (!Engine.ResourceManager->_GetDetailTexture(base,detail_texture,detail_scaler))	bDetail	= FALSE;
		if (!Engine.ResourceManager->m_textures_description.GetDetailTexture(base, detail_texture, detail_scaler))
			bDetail = FALSE;
	}
	else
	{
		bDetail = FALSE;
	}

	// Validate for R1 or R2
	bDetail_Diffuse = FALSE;
	bDetail_Bump = FALSE;
	if (bDetail)
		Engine.ResourceManager->m_textures_description.GetTextureUsage(base, bDetail_Diffuse, bDetail_Bump);

	bSteepParallax = FALSE;
	Engine.ResourceManager->m_textures_description.GetParallax(base, bSteepParallax);

	// Compile
	BT->Compile(*this);
}

void CBlender_Compile::SetParams(int iPriority, bool bStrictB2F)
{
	SH->flags.iPriority = iPriority;
	SH->flags.bStrictB2F = bStrictB2F;
	if (bStrictB2F)
	{
#ifdef _EDITOR
		if (1 != (SH->flags.iPriority / 2))
		{
			Log("!If StrictB2F true then Priority must div 2.");
			SH->flags.bStrictB2F = FALSE;
		}
#else
		//VERIFY(1 == (SH->flags.iPriority / 2));
#endif
	}
	// SH->Flags.bLighting		= FALSE;
}

void CBlender_Compile::PassBegin()
{
	RS.Invalidate();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	strcpy_s(pass_ps, "null");
	strcpy_s(pass_vs, "null");
	strcpy_s(pass_ps_entry, "main");
	strcpy_s(pass_vs_entry, "main");
	dwStage = 0;

	// Очищаем все списки
	macros_common.clear();
	macros_vs.clear();
	macros_ps.clear();
}

void CBlender_Compile::PassEnd()
{
	// Last Stage - disable
	RS.SetTSS(Stage(), D3DTSS_COLOROP, D3DTOP_DISABLE);
	RS.SetTSS(Stage(), D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	// Create pass
	ref_state state = Engine.ResourceManager->_CreateState(RS.GetContainer());

	// [НОВАЯ ЛОГИКА] Объединяем макросы
	// 1. Подготавливаем макросы для Pixel Shader (Common + PS)
	CShaderMacros final_macros_ps;
	final_macros_ps.add(macros_common);
	final_macros_ps.add(macros_ps);

	// 2. Подготавливаем макросы для Vertex Shader (Common + VS)
	CShaderMacros final_macros_vs;
	final_macros_vs.add(macros_common);
	final_macros_vs.add(macros_vs);

	// Компилируем шейдеры с соответствующими макросами
	ref_ps ps = Engine.ResourceManager->CreateShader<SPS>(pass_ps, pass_ps_entry, final_macros_ps);
	ref_vs vs = Engine.ResourceManager->CreateShader<SVS>(pass_vs, pass_vs_entry, final_macros_vs);

	// Очищаем списки после создания прохода
	macros_common.clear();
	macros_vs.clear();
	macros_ps.clear();

	ctable.merge(&ps->constants);
	ctable.merge(&vs->constants);
	SetMapping();
	ref_ctable ct = Engine.ResourceManager->_CreateConstantTable(ctable);
	ref_texture_list T = Engine.ResourceManager->_CreateTextureList(passTextures);
	ref_matrix_list M = Engine.ResourceManager->_CreateMatrixList(passMatrices);
	ref_constant_list C = Engine.ResourceManager->_CreateConstantList(passConstants);

	ref_pass _pass_ = Engine.ResourceManager->_CreatePass(state, ps, vs, ct, T, M, C);
	SH->passes.push_back(_pass_);
}

void CBlender_Compile::PassSET_PS(LPCSTR name, LPCSTR entry)
{
	strcpy_s(pass_ps, name);
	xr_strlwr(pass_ps);

	// Если entry задан, копируем, иначе main
	if (entry && entry[0])
		strcpy_s(pass_ps_entry, entry);
	else
		strcpy_s(pass_ps_entry, "main");
}

void CBlender_Compile::PassSET_VS(LPCSTR name, LPCSTR entry)
{
	strcpy_s(pass_vs, name);
	xr_strlwr(pass_vs);

	// Если entry задан, копируем, иначе main
	if (entry && entry[0])
		strcpy_s(pass_vs_entry, entry);
	else
		strcpy_s(pass_vs_entry, "main");
}

void CBlender_Compile::PassSET_ZB(BOOL bZTest, BOOL bZWrite, BOOL bInvertZTest)
{
	if (Pass())
		bZWrite = FALSE;
	RS.SetRS(D3DRS_ZFUNC, bZTest ? (bInvertZTest ? D3DCMP_GREATER : D3DCMP_LESSEQUAL) : D3DCMP_ALWAYS);
	RS.SetRS(D3DRS_ZWRITEENABLE, BC(bZWrite));
}

void CBlender_Compile::PassSET_ablend_mode(BOOL bABlend, u32 abSRC, u32 abDST)
{
	if (bABlend && D3DBLEND_ONE == abSRC && D3DBLEND_ZERO == abDST)
		bABlend = FALSE;
	RS.SetRS(D3DRS_ALPHABLENDENABLE, BC(bABlend));
	RS.SetRS(D3DRS_SRCBLEND, bABlend ? abSRC : D3DBLEND_ONE);
	RS.SetRS(D3DRS_DESTBLEND, bABlend ? abDST : D3DBLEND_ZERO);
}

void CBlender_Compile::PassSET_ablend_aref(BOOL bATest, u32 aRef)
{
	clamp(aRef, 0u, 255u);
	RS.SetRS(D3DRS_ALPHATESTENABLE, BC(bATest));
	if (bATest)
		RS.SetRS(D3DRS_ALPHAREF, u32(aRef));
}

void CBlender_Compile::PassSET_Blend(BOOL bABlend, u32 abSRC, u32 abDST, BOOL bATest, u32 aRef)
{
	PassSET_ablend_mode(bABlend, abSRC, abDST);
#ifdef DEBUG
	if (strstr(Core.Params, "-noaref"))
	{
		bATest = FALSE;
		aRef = 0;
	}
#endif
	PassSET_ablend_aref(bATest, aRef);
}

void CBlender_Compile::PassSET_LightFog(BOOL bLight, BOOL bFog)
{
	RS.SetRS(D3DRS_LIGHTING, BC(bLight));
	RS.SetRS(D3DRS_FOGENABLE, BC(bFog));
	// SH->Flags.bLighting				|= !!bLight;
}

void CBlender_Compile::StageBegin()
{
	StageSET_Address(D3DTADDRESS_WRAP); // Wrapping enabled by default
}

void CBlender_Compile::StageEnd()
{
	dwStage++;
}
void CBlender_Compile::StageSET_Address(u32 adr)
{
	RS.SetSAMP(Stage(), D3DSAMP_ADDRESSU, adr);
	RS.SetSAMP(Stage(), D3DSAMP_ADDRESSV, adr);
}

void CBlender_Compile::StageSET_Transform(u32 tf, u32 tc)
{
#ifdef _EDITOR
	RS.SetTSS(Stage(), D3DTSS_TEXTURETRANSFORMFLAGS, tf);
	RS.SetTSS(Stage(), D3DTSS_TEXCOORDINDEX, tc);
#endif
}

void CBlender_Compile::StageSET_Color(u32 a1, u32 op, u32 a2)
{
	RS.SetColor(Stage(), a1, op, a2);
}

void CBlender_Compile::StageSET_Color3(u32 a1, u32 op, u32 a2, u32 a3)
{
	RS.SetColor3(Stage(), a1, op, a2, a3);
}

void CBlender_Compile::StageSET_Alpha(u32 a1, u32 op, u32 a2)
{
	RS.SetAlpha(Stage(), a1, op, a2);
}

void CBlender_Compile::StageSET_TMC(LPCSTR T, LPCSTR M, LPCSTR C, int UVW_channel)
{
	Stage_Texture(T);
	Stage_Matrix(M, UVW_channel);
	Stage_Constant(C);
}

void CBlender_Compile::StageTemplate_LMAP0()
{
	StageSET_Address(D3DTADDRESS_CLAMP);
	StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
	StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
	StageSET_TMC("$base1", "$null", "$null", 1);
}

void CBlender_Compile::Stage_Texture(LPCSTR name, u32, u32 fmin, u32 fmip, u32 fmag)
{
	sh_list& lst = L_textures;
	int id = ParseName(name);
	LPCSTR N = name;
	if (id >= 0)
	{
		if (id >= int(lst.size()))
			Debug.fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
		N = *lst[id];
	}
	passTextures.push_back(mk_pair(Stage(), ref_texture(Engine.ResourceManager->_CreateTexture(N))));
	//	i_Address				(Stage(),address);
	i_Filter(Stage(), fmin, fmip, fmag);
}

void CBlender_Compile::Stage_Matrix(LPCSTR name, int iChannel)
{
	sh_list& lst = L_matrices;
	int id = ParseName(name);
	CMatrix* M = Engine.ResourceManager->_CreateMatrix((id >= 0) ? *lst[id] : name);
	passMatrices.push_back(M);

	// Setup transform pipeline
	u32 ID = Stage();
	if (M)
	{
		switch (M->dwMode)
		{
		case CMatrix::modeProgrammable:
			StageSET_Transform(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEPOSITION | ID);
			break;
		case CMatrix::modeTCM:
			StageSET_Transform(D3DTTFF_COUNT2, D3DTSS_TCI_PASSTHRU | iChannel);
			break;
		case CMatrix::modeC_refl:
			StageSET_Transform(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | ID);
			break;
		case CMatrix::modeS_refl:
			StageSET_Transform(D3DTTFF_COUNT2, D3DTSS_TCI_CAMERASPACENORMAL | ID);
			break;
		default:
			StageSET_Transform(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel);
			break;
		}
	}
	else
	{
		// No Transform at all
		StageSET_Transform(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel);
	}
}
void CBlender_Compile::Stage_Constant(LPCSTR name)
{
	sh_list& lst = L_constants;
	int id = ParseName(name);
	passConstants.push_back(Engine.ResourceManager->_CreateConstant((id >= 0) ? *lst[id] : name));
}

void CBlender_Compile::begin_Pass(LPCSTR _vs, LPCSTR _ps, LPCSTR _vs_entry, LPCSTR _ps_entry, bool bFog, BOOL bZtest,
								  BOOL bZwrite, BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	// 1. Сначала копируем имена. Если пришли NULL - ставим "null".
	strcpy_s(pass_vs, _vs ? _vs : "null");
	strcpy_s(pass_ps, _ps ? _ps : "null");
	strcpy_s(pass_vs_entry, _vs_entry ? _vs_entry : "main");
	strcpy_s(pass_ps_entry, _ps_entry ? _ps_entry : "main");

	// 2. ОБЯЗАТЕЛЬНО коммитим СРАЗУ.
	// Это подготовит базовый проход и ctable для сэмплеров.
	// При этом внутри вызовется RS.Invalidate(), что нормально, т.к. мы еще не настроили стейты.
	commit_Pass();

	// 3. ПРИМЕНЯЕМ НАСТРОЙКИ СТЕЙТОВ ПОСЛЕ КОММИТА
	// Теперь они лягут в чистый RS и не сотрутся.
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);
}

void CBlender_Compile::commit_Pass()
{
	// Сброс эмулятора
	//RS.Invalidate();

	//ctable.clear();
	//passTextures.clear();
	//passMatrices.clear();
	//passConstants.clear();
	dwStage = 0;

	// Макросы
	CShaderMacros final_macros_ps;
	final_macros_ps.add(macros_common);
	final_macros_ps.add(macros_ps);

	CShaderMacros final_macros_vs;
	final_macros_vs.add(macros_common);
	final_macros_vs.add(macros_vs);

	// Создание шейдеров
	ref_ps ps = Engine.ResourceManager->CreateShader<SPS>(pass_ps, pass_ps_entry, final_macros_ps);
	ref_vs vs = Engine.ResourceManager->CreateShader<SVS>(pass_vs, pass_vs_entry, final_macros_vs);

	// Очистка
	macros_common.clear();
	macros_vs.clear();
	macros_ps.clear();

	// Сохранение в дест
	dest.ps = ps;
	dest.vs = vs;

	// Проверяем, создались ли шейдеры, прежде чем лезть в их константы.
	// Если шейдер "null", реф-каунтер (p_) будет равен nullptr.
	if (ps)
		ctable.merge(&ps->constants);
	if (vs)
		ctable.merge(&vs->constants);

	// SetMapping привязывает сэмплеры к симулятору.
	// Важно: он использует dest.ps, который теперь безопасно обновлен (даже если он пуст).
	SetMapping();

	// Отключение последнего стейджа
	if (0 == xr_stricmp(pass_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::set_Constant(LPCSTR name, R_constant_setup* s)
{
	R_ASSERT(s);
	ref_constant C = ctable.get(name);
	if (C)
		C->handler = s;
}

u32 CBlender_Compile::i_Sampler(LPCSTR _name)
{
	//
	string256 name;
	strcpy_s(name, _name);
	//. andy	if (strext(name)) *strext(name)=0;
	Engine.ResourceManager->fix_texture_name(name);

	// Find index
	ref_constant C = ctable.get(name);
	if (!C)
		return u32(-1);
	R_ASSERT(C->type == RC_sampler);
	u32 stage = C->samp.index;

	// Create texture
	// while (stage>=passTextures.size())	passTextures.push_back		(NULL);
	return stage;
}

void CBlender_Compile::i_Texture(u32 s, LPCSTR name)
{
	if (name)
		passTextures.push_back(mk_pair(s, ref_texture(Engine.ResourceManager->_CreateTexture(name))));
}

void CBlender_Compile::i_Projective(u32 s, bool b)
{
	if (b)
		RS.SetTSS(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE | D3DTTFF_PROJECTED);
	else
		RS.SetTSS(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void CBlender_Compile::i_Address(u32 s, u32 address)
{
	RS.SetSAMP(s, D3DSAMP_ADDRESSU, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSV, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSW, address);
}

void CBlender_Compile::i_BorderColor(u32 s, u32 color)
{
	RS.SetSAMP(s, D3DSAMP_BORDERCOLOR, color);
}

void CBlender_Compile::i_Filter_Min(u32 s, u32 f)
{
	RS.SetSAMP(s, D3DSAMP_MINFILTER, f);
}

void CBlender_Compile::i_Filter_Mip(u32 s, u32 f)
{
	RS.SetSAMP(s, D3DSAMP_MIPFILTER, f);
}

void CBlender_Compile::i_Filter_Mag(u32 s, u32 f)
{
	RS.SetSAMP(s, D3DSAMP_MAGFILTER, f);
}

void CBlender_Compile::i_Filter(u32 s, u32 _min, u32 _mip, u32 _mag)
{
	i_Filter_Min(s, _min);
	i_Filter_Mip(s, _mip);
	i_Filter_Mag(s, _mag);
}

u32 CBlender_Compile::set_Sampler(LPCSTR _name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, u32 address, u32 fmin,
								u32 fmip, u32 fmag, bool b_srgb)
{
	dwStage = i_Sampler(_name);
	if (u32(-1) != dwStage)
	{
		i_Texture(dwStage, texture);

		// force ANISO-TF for "s_base"
		if ((0 == xr_strcmp(_name, "s_base")) && (fmin == D3DTEXF_LINEAR))
		{
			fmin = D3DTEXF_ANISOTROPIC;
			fmag = D3DTEXF_ANISOTROPIC;
		}
		if ((0 == xr_strcmp(_name, "s_detail")) && (fmin == D3DTEXF_LINEAR))
		{
			fmin = D3DTEXF_ANISOTROPIC;
			fmag = D3DTEXF_ANISOTROPIC;
		}

		// Sampler states
		i_Address(dwStage, address);
		i_Filter(dwStage, fmin, fmip, fmag);

		if (dwStage < 4)
			i_Projective(dwStage, b_ps1x_ProjectiveDivide);
	}
	return dwStage;
}

void CBlender_Compile::set_Sampler_point(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, bool b_SRGB)
{
	set_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT,
			  b_SRGB);
}

void CBlender_Compile::set_Sampler_linear(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, bool b_SRGB)
{
	set_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR,
			  b_SRGB);
}

void CBlender_Compile::set_Sampler_linear_wrap(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, bool b_SRGB)
{
	u32 s = set_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE,
					  D3DTEXF_LINEAR, b_SRGB);
	if (u32(-1) != s)
		RS.SetSAMP(s, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
}

void CBlender_Compile::set_Sampler_point_wrap(LPCSTR name, LPCSTR texture, bool b_SRGB)
{
	set_Sampler(name, texture, false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}

void CBlender_Compile::set_Sampler_gaussian(LPCSTR name, LPCSTR texture, bool b_SRGB)
{
	set_Sampler(name, texture, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_NONE, D3DTEXF_GAUSSIANQUAD, b_SRGB);
}

void CBlender_Compile::end_Pass()
{
	dest.constants = Engine.ResourceManager->_CreateConstantTable(ctable);
	dest.state = Engine.ResourceManager->_CreateState(RS.GetContainer());
	dest.T = Engine.ResourceManager->_CreateTextureList(passTextures);
	dest.C = 0;

	ref_matrix_list temp(0);
	SH->passes.push_back(
		Engine.ResourceManager->_CreatePass(dest.state, dest.ps, dest.vs, dest.constants, dest.T, temp, dest.C));

	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
}
