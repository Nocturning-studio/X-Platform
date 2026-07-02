#include "stdafx.h"
#pragma once

#ifndef _EDITOR
#include "render.h"
#endif
#include "Thunderbolt.h"
#include "igame_persistent.h"
#include "LightAnimLibrary.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "xr_area.h"
#include "xr_object.h"
#endif

static const float MAX_DIST_FACTOR = 0.95f;

SThunderboltDesc::SThunderboltDesc() : m_GradientTop(0), m_GradientCenter(0)
{
}

SThunderboltDesc::~SThunderboltDesc()
{
	::Render->model_Delete(l_model);
	m_GradientTop->hShader.destroy();
	m_GradientCenter->hShader.destroy();
	snd.destroy();

	xr_delete(m_GradientTop);
	xr_delete(m_GradientCenter);
}

void SThunderboltDesc::create_top_gradient(CInifile& pIni, shared_str const& sect)
{
	m_GradientTop = xr_new<SFlare>();
	m_GradientTop->shader = pIni.r_string(sect, "gradient_top_shader");
	m_GradientTop->texture = pIni.r_string(sect, "gradient_top_texture");
	m_GradientTop->fRadius = pIni.r_fvector2(sect, "gradient_top_radius");
	m_GradientTop->fOpacity = pIni.r_float(sect, "gradient_top_opacity");
	m_GradientTop->hShader.create(*m_GradientTop->shader, *m_GradientTop->texture);
}

void SThunderboltDesc::create_center_gradient(CInifile& pIni, shared_str const& sect)
{
	m_GradientCenter = xr_new<SFlare>();
	m_GradientCenter->shader = pIni.r_string(sect, "gradient_center_shader");
	m_GradientCenter->texture = pIni.r_string(sect, "gradient_center_texture");
	m_GradientCenter->fRadius = pIni.r_fvector2(sect, "gradient_center_radius");
	m_GradientCenter->fOpacity = pIni.r_float(sect, "gradient_center_opacity");
	m_GradientCenter->hShader.create(*m_GradientCenter->shader, *m_GradientCenter->texture);
}

void SThunderboltDesc::load(CInifile& pIni, shared_str const& sect)
{
	create_top_gradient(pIni, sect);
	create_center_gradient(pIni, sect);

	name = sect;
	color_anim = LALib.FindItem(pIni.r_string(sect, "color_anim"));
	VERIFY(color_anim);
	color_anim->fFPS = (float)color_anim->iFrameCount;

	// models
	LPCSTR m_name;
	m_name = pIni.r_string(sect, "lightning_model");
	IReader* F = nullptr;
	F = FS.r_open("$game_meshes$", m_name);

	if(!F)
		Msg("Empty 'lightning_model' in section %s", sect.c_str());

	l_model = ::Render->model_CreateDM(F);
	FS.r_close(F);

	// sound
	m_name = pIni.r_string(sect, "sound");
	if (m_name && m_name[0])
		snd.create(m_name, st_Effect, sg_Undefined);
}

//----------------------------------------------------------------------------------------------
// collection
//----------------------------------------------------------------------------------------------
SThunderboltCollection::SThunderboltCollection()
{
}

void SThunderboltCollection::load(CInifile* pIni, CInifile* thunderbolts, LPCSTR sect)
{
	section = sect;
	int tb_count = pIni->line_count(sect);
	for (int tb_idx = 0; tb_idx < tb_count; tb_idx++)
	{
		LPCSTR N, V;
		if (pIni->r_line(sect, tb_idx, &N, &V))
			palette.push_back(g_pGamePersistent->Environment().thunderbolt_description(*thunderbolts, N));
	}
}
SThunderboltCollection::~SThunderboltCollection()
{
	for (DescIt d_it = palette.begin(); d_it != palette.end(); d_it++)
		xr_delete(*d_it);

	palette.clear();
}

//----------------------------------------------------------------------------------------------
// thunderbolt effect
//----------------------------------------------------------------------------------------------
CEffect_Thunderbolt::CEffect_Thunderbolt()
{
	current = 0;
	life_time = 0.f;
	state = stIdle;
	next_lightning_time = 0.f;
	bEnabled = FALSE;

	// geom
	hGeom_model.create(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, RenderBackend.Vertex.Buffer(), RenderBackend.Index.Buffer());
	hGeom_gradient.create(FVF::F_LIT, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);
}

CEffect_Thunderbolt::~CEffect_Thunderbolt()
{
	for (CollectionVecIt d_it = collection.begin(); d_it != collection.end(); d_it++)
		xr_delete(*d_it);
	collection.clear();
	hGeom_model.destroy();
	hGeom_gradient.destroy();
}

shared_str CEffect_Thunderbolt::AppendDef(CEnvironment& environment, CInifile* pIni, CInifile* thunderbolts,
										  LPCSTR sect)
{
	if (!sect || (0 == sect[0]))
		return "";
	for (CollectionVecIt it = collection.begin(); it != collection.end(); it++)
		if ((*it)->section == sect)
			return (*it)->section;
	collection.push_back(environment.thunderbolt_collection(pIni, thunderbolts, sect));
	return collection.back()->section;
}

BOOL CEffect_Thunderbolt::RayPick(const fvec3& s, const fvec3& d, float& dist)
{
	BOOL bRes = TRUE;
#ifdef _EDITOR
	bRes = Tools->RayPick(s, d, dist, 0, 0);
#else
	collide::rq_result RQ;
	CObject* E = g_pGameLevel->CurrentViewEntity();
	bRes = g_pGameLevel->ObjectSpace.RayPick(s, d, dist, collide::rqtBoth, RQ, E);
	if (bRes)
		dist = RQ.range;
	else
	{
		fvec3 N = {0.f, -1.f, 0.f};
		fvec3 P = {0.f, 0.f, 0.f};
		Fplane PL;
		PL.build(P, N);
		float dst = dist;
		if (PL.intersectRayDist(s, d, dst) && (dst <= dist))
		{
			dist = dst;
			return true;
		}
		else
			return false;
	}
#endif
	return bRes;
}
#define FAR_DIST g_pGamePersistent->Environment().CurrentEnv->far_plane

void CEffect_Thunderbolt::Bolt(shared_str id, float period, float lt)
{
	VERIFY(id.size());
	state = stWorking;
	life_time = lt + Random.randF(-lt * 0.5f, lt * 0.5f);
	current_time = 0.f;

	current = g_pGamePersistent->Environment().thunderbolt_collection(collection, id)->GetRandomDesc();
	VERIFY(current);

	fmat4x4 XF, S;
	fvec3 pos, dev;
	float sun_h, sun_p;
	CEnvironment& environment = g_pGamePersistent->Environment();
	environment.CurrentEnv->sun_dir.getHP(sun_h, sun_p);
	float alt = environment.p_var_alt; // Random.randF(environment.p_var_alt.x,environment.p_var_alt.y);
	float lng = Random.randF(sun_h - environment.p_var_long + PI, sun_h + environment.p_var_long + PI);
	float dist = Random.randF(FAR_DIST * environment.p_min_dist, FAR_DIST * .95f);
	current_direction.setHP(lng, alt);
	pos.mad(Engine.RenderView.Position, current_direction, dist);
	dev.x = Random.randF(-environment.p_tilt, environment.p_tilt);
	dev.y = Random.randF(0, PI_MUL_2);
	dev.z = Random.randF(-environment.p_tilt, environment.p_tilt);
	XF.setXYZi(dev);

	fvec3 light_dir = {0.f, -1.f, 0.f};
	XF.transform_dir(light_dir);
	lightning_size = FAR_DIST * 2.f;
	RayPick(pos, light_dir, lightning_size);

	lightning_center.mad(pos, light_dir, lightning_size * 0.5f);

	S.scale(lightning_size, lightning_size, lightning_size);
	XF.translate_over(pos);
	current_transform.mul_43(XF, S);

	float next_v = Random.randF();

	if (next_v < environment.p_second_prop)
	{
		next_lightning_time = Engine.TimeManager.GetGlobalTime() + lt + EPS_L;
	}
	else
	{
		next_lightning_time = Engine.TimeManager.GetGlobalTime() + period + Random.randF(-period * 0.3f, period * 0.3f);
		current->snd.play_no_feedback(0, 0, dist / 300.f, &pos, 0, 0, &fvec2().set(dist / 2, dist * 2.f));
	}

	current_direction.invert(); // for env-sun
}

void CEffect_Thunderbolt::OnFrame(shared_str id, float period, float duration)
{
	BOOL enabled = !!(id.size());
	if (bEnabled != enabled)
	{
		bEnabled = enabled;
		next_lightning_time = Engine.TimeManager.GetGlobalTime() + period + Random.randF(-period * 0.5f, period * 0.5f);
	}
	else if (bEnabled && (Engine.TimeManager.GetGlobalTime() > next_lightning_time))
	{
		if (state == stIdle && !!(id.size()))
			Bolt(id, period, duration);
	}
	if (state == stWorking)
	{
		if (current_time > life_time)
			state = stIdle;
		current_time += Engine.TimeManager.GetDeltaTime();
		fvec3 fClr;
		int frame;
		u32 uClr = current->color_anim->CalculateRGB(current_time / life_time, frame);
		fClr.set(clampr(float(color_get_R(uClr) / 255.f), 0.f, 1.f), clampr(float(color_get_G(uClr) / 255.f), 0.f, 1.f),
				 clampr(float(color_get_B(uClr) / 255.f), 0.f, 1.f));

		lightning_phase = 1.5f * (current_time / life_time);
		clamp(lightning_phase, 0.f, 1.f);

		CEnvironment& environment = g_pGamePersistent->Environment();

		fvec3& sky_color = environment.CurrentEnv->sky_color;
		sky_color.mad(fClr, environment.p_sky_color);
		clamp(sky_color.x, 0.f, 1.f);
		clamp(sky_color.y, 0.f, 1.f);
		clamp(sky_color.z, 0.f, 1.f);

		environment.CurrentEnv->sun_color.mad(fClr, environment.p_sun_color);
		environment.CurrentEnv->fog_color.mad(fClr, environment.p_fog_color);

		R_ASSERT(_valid(current_direction));
		g_pGamePersistent->Environment().CurrentEnv->sun_dir = current_direction;
		VERIFY2(g_pGamePersistent->Environment().CurrentEnv->sun_dir.y < 0,
					"Invalid sun direction settings while CEffect_Thunderbolt");
	}
}

void CEffect_Thunderbolt::Render()
{
	//OPTICK_EVENT("CEffect_Thunderbolt::Render");

	if (state == stWorking)
	{
		VERIFY(current);

		// lightning model
		float dv = lightning_phase * 0.5f;
		dv = (lightning_phase > 0.5f) ? Random.randI(2) * 0.5f : dv;

		RenderBackend.set_CullMode(CULL_DISABLE);
		u32 v_offset, i_offset;
		u32 vCount_Lock = current->l_model->number_vertices;
		u32 iCount_Lock = current->l_model->number_indices;
		IRender_DetailModel::fvfVertexOut* v_ptr =
			(IRender_DetailModel::fvfVertexOut*)RenderBackend.Vertex.Lock(vCount_Lock, hGeom_model->vb_stride, v_offset);
		u16* i_ptr = RenderBackend.Index.Lock(iCount_Lock, i_offset);
		// Transform verts
		current->l_model->transfer(current_transform, v_ptr, 0xffffffff, i_ptr, 0, 0.f, dv);
		// Flush if needed
		RenderBackend.Vertex.Unlock(vCount_Lock, hGeom_model->vb_stride);
		RenderBackend.Index.Unlock(iCount_Lock);
		RenderBackend.set_transform_world(Fidentity);
		RenderBackend.set_Shader(current->l_model->shader);
		RenderBackend.set_Geometry(hGeom_model);
		RenderBackend.Render(D3DPT_TRIANGLELIST, v_offset, 0, vCount_Lock, i_offset, iCount_Lock / 3);
		RenderBackend.set_CullMode(CULL_BACKFACE);

		// gradient
		fvec3 vecSx, vecSy;
		u32 VS_Offset;
		FVF::LIT* pv = (FVF::LIT*)RenderBackend.Vertex.Lock(8, hGeom_gradient.stride(), VS_Offset);
		// top
		{
			u32 c_val = iFloor(current->m_GradientTop->fOpacity * lightning_phase * 255.f);
			u32 c = color_rgba(c_val, c_val, c_val, c_val);
			vecSx.mul(Engine.RenderView.Right, current->m_GradientTop->fRadius.x * lightning_size);
			vecSy.mul(Engine.RenderView.Top, -current->m_GradientTop->fRadius.y * lightning_size);
			pv->set(current_transform.c.x + vecSx.x - vecSy.x, current_transform.c.y + vecSx.y - vecSy.y,
					current_transform.c.z + vecSx.z - vecSy.z, c, 0, 0);
			pv++;
			pv->set(current_transform.c.x + vecSx.x + vecSy.x, current_transform.c.y + vecSx.y + vecSy.y,
					current_transform.c.z + vecSx.z + vecSy.z, c, 0, 1);
			pv++;
			pv->set(current_transform.c.x - vecSx.x - vecSy.x, current_transform.c.y - vecSx.y - vecSy.y,
					current_transform.c.z - vecSx.z - vecSy.z, c, 1, 0);
			pv++;
			pv->set(current_transform.c.x - vecSx.x + vecSy.x, current_transform.c.y - vecSx.y + vecSy.y,
					current_transform.c.z - vecSx.z + vecSy.z, c, 1, 1);
			pv++;
		}
		// center
		{
			u32 c_val = iFloor(current->m_GradientTop->fOpacity * lightning_phase * 255.f);
			u32 c = color_rgba(c_val, c_val, c_val, c_val);
			vecSx.mul(Engine.RenderView.Right, current->m_GradientCenter->fRadius.x * lightning_size);
			vecSy.mul(Engine.RenderView.Top, -current->m_GradientCenter->fRadius.y * lightning_size);
			pv->set(lightning_center.x + vecSx.x - vecSy.x, lightning_center.y + vecSx.y - vecSy.y,
					lightning_center.z + vecSx.z - vecSy.z, c, 0, 0);
			pv++;
			pv->set(lightning_center.x + vecSx.x + vecSy.x, lightning_center.y + vecSx.y + vecSy.y,
					lightning_center.z + vecSx.z + vecSy.z, c, 0, 1);
			pv++;
			pv->set(lightning_center.x - vecSx.x - vecSy.x, lightning_center.y - vecSx.y - vecSy.y,
					lightning_center.z - vecSx.z - vecSy.z, c, 1, 0);
			pv++;
			pv->set(lightning_center.x - vecSx.x + vecSy.x, lightning_center.y - vecSx.y + vecSy.y,
					lightning_center.z - vecSx.z + vecSy.z, c, 1, 1);
			pv++;
		}
		RenderBackend.Vertex.Unlock(8, hGeom_gradient.stride());
		RenderBackend.set_transform_world(Fidentity);
		RenderBackend.set_Geometry(hGeom_gradient);
		RenderBackend.set_Shader(current->m_GradientTop->hShader);
		RenderBackend.Render(D3DPT_TRIANGLELIST, VS_Offset, 0, 4, 0, 2);
		RenderBackend.set_Shader(current->m_GradientCenter->hShader);
		RenderBackend.Render(D3DPT_TRIANGLELIST, VS_Offset + 4, 0, 4, 0, 2);
	}
}
