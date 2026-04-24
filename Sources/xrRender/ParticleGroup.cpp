//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "..\xrEngine\psystem.h"
#include "ParticleGroup.h"
#include "PSLibrary.h"

using namespace PS;

//------------------------------------------------------------------------------
CPGDef::CPGDef()
{
	m_Flags.zero();
	m_fTimeLimit = 0.f;
}

CPGDef::~CPGDef()
{
	for (EffectIt it = m_Effects.begin(); it != m_Effects.end(); it++)
		xr_delete(*it);
	m_Effects.clear();
}

void CPGDef::SetName(LPCSTR name)
{
	m_Name = name;
}

#ifdef _EDITOR
void CPGDef::Clone(CPGDef* source)
{
	m_Name = "<invalid_name>";
	m_Flags = source->m_Flags;
	m_fTimeLimit = source->m_fTimeLimit;
	m_OwnerName = source->m_OwnerName;
	m_ModifName = source->m_ModifName;
	m_CreateTime = source->m_CreateTime;
	m_ModifTime = source->m_ModifTime;

	m_Effects.resize(source->m_Effects.size(), 0);
	for (EffectIt d_it = m_Effects.begin(), s_it = source->m_Effects.begin(); s_it != source->m_Effects.end();
		 s_it++, d_it++)
		*d_it = xr_new<SEffect>(**s_it);
}
#endif

//------------------------------------------------------------------------------
// I/O part
//------------------------------------------------------------------------------
BOOL CPGDef::Load(IReader& F)
{
	R_ASSERT(F.find_chunk(PGD_CHUNK_VERSION));
	u16 version = F.r_u16();

	if (version != PGD_VERSION)
	{
		Log("!Unsupported PG version. Load failed.");
		return FALSE;
	}

	R_ASSERT(F.find_chunk(PGD_CHUNK_NAME));
	F.r_stringZ(m_Name);

	F.r_chunk(PGD_CHUNK_FLAGS, &m_Flags);

	if (F.find_chunk(PGD_CHUNK_EFFECTS))
	{
		m_Effects.resize(F.r_u32());
		for (EffectIt it = m_Effects.begin(); it != m_Effects.end(); it++)
		{
			*it = xr_new<SEffect>();
			F.r_stringZ((*it)->m_EffectName);
			F.r_stringZ((*it)->m_OnPlayChildName);
			F.r_stringZ((*it)->m_OnBirthChildName);
			F.r_stringZ((*it)->m_OnDeadChildName);
			(*it)->m_Time0 = F.r_float();
			(*it)->m_Time1 = F.r_float();
			(*it)->m_Flags.assign(F.r_u32());
		}
	}
	else
	{ //.??? убрать через некоторое время
		R_ASSERT(F.find_chunk(PGD_CHUNK_EFFECTS2));
		m_Effects.resize(F.r_u32());
		for (EffectIt it = m_Effects.begin(); it != m_Effects.end(); it++)
		{
			*it = xr_new<SEffect>();
			F.r_stringZ((*it)->m_EffectName);
			F.r_stringZ((*it)->m_OnPlayChildName);
			(*it)->m_Time0 = F.r_float();
			(*it)->m_Time1 = F.r_float();
			(*it)->m_Flags.assign(F.r_u32());
		}
	}

	if (F.find_chunk(PGD_CHUNK_TIME_LIMIT))
	{
		m_fTimeLimit = F.r_float();
	}

#ifdef _EDITOR
	if (F.find_chunk(PGD_CHUNK_OWNER))
	{
		F.r_stringZ(m_OwnerName);
		F.r_stringZ(m_ModifName);
		F.r(&m_CreateTime, sizeof(m_CreateTime));
		F.r(&m_ModifTime, sizeof(m_ModifTime));
	}
#endif

	return TRUE;
}

void CPGDef::Save(IWriter& F)
{
	F.open_chunk(PGD_CHUNK_VERSION);
	F.w_u16(PGD_VERSION);
	F.close_chunk();

	F.open_chunk(PGD_CHUNK_NAME);
	F.w_stringZ(m_Name);
	F.close_chunk();

	F.w_chunk(PGD_CHUNK_FLAGS, &m_Flags, sizeof(m_Flags));

	F.open_chunk(PGD_CHUNK_EFFECTS);
	F.w_u32(m_Effects.size());
	for (EffectIt it = m_Effects.begin(); it != m_Effects.end(); it++)
	{
		F.w_stringZ((*it)->m_EffectName);
		F.w_stringZ((*it)->m_OnPlayChildName);
		F.w_stringZ((*it)->m_OnBirthChildName);
		F.w_stringZ((*it)->m_OnDeadChildName);
		F.w_float((*it)->m_Time0);
		F.w_float((*it)->m_Time1);
		F.w_u32((*it)->m_Flags.get());
	}
	F.close_chunk();

	F.open_chunk(PGD_CHUNK_TIME_LIMIT);
	F.w_float(m_fTimeLimit);
	F.close_chunk();

#ifdef _EDITOR
	F.open_chunk(PGD_CHUNK_OWNER);
	F.w_stringZ(m_OwnerName);
	F.w_stringZ(m_ModifName);
	F.w(&m_CreateTime, sizeof(m_CreateTime));
	F.w(&m_ModifTime, sizeof(m_ModifTime));
	F.close_chunk();
#endif
}

//------------------------------------------------------------------------------
// Particle Group item
//------------------------------------------------------------------------------
void CParticleGroup::SItem::Set(IRender_Visual* e)
{
	_effect = e;
}
void CParticleGroup::SItem::Clear()
{
	VisualVec visuals;
	GetVisuals(visuals);
	for (VisualVecIt it = visuals.begin(); it != visuals.end(); it++)
		::Render->model_Delete(*it);
}
void CParticleGroup::SItem::StartRelatedChild(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m)
{
	CParticleEffect* C = static_cast<CParticleEffect*>(RenderImplementation.model_CreatePE(eff_name));
	float4x4 M;
	M.identity();
	float3 vel;
	vel.sub(m.pos, m.posB);
	vel.div(fDT_STEP);
	if (emitter->m_RT_Flags.is(CParticleEffect::flRT_Transform))
	{
		M.set(emitter->m_Transform);
		M.transform_dir(vel);
	};
	float3 p;
	M.transform_tiny(p, m.pos);
	M.c.set(p);
	C->Play();
	C->UpdateParent(M, vel, FALSE);
	_children_related.push_back(C);
}
void CParticleGroup::SItem::StopRelatedChild(u32 idx)
{
	VERIFY(idx < _children_related.size());
	IRender_Visual*& V = _children_related[idx];
	((CParticleEffect*)V)->Stop(TRUE);
	_children_free_pending.push_back(V);   // вместо прямого push_back
	_children_related[idx] = _children_related.back();
	_children_related.pop_back();
}
void CParticleGroup::SItem::StartFreeChild(CParticleEffect* emitter, LPCSTR nm, PAPI::Particle& m)
{
	CParticleEffect* C = static_cast<CParticleEffect*>(RenderImplementation.model_CreatePE(nm));
	if (!C->IsLooped())
	{
		float4x4 M;
		M.identity();
		float3 vel;
		vel.sub(m.pos, m.posB);
		vel.div(fDT_STEP);
		if (emitter->m_RT_Flags.is(CParticleEffect::flRT_Transform))
		{
			M.set(emitter->m_Transform);
			M.transform_dir(vel);
		};
		float3 p;
		M.transform_tiny(p, m.pos);
		M.c.set(p);
		C->Play();
		C->UpdateParent(M, vel, FALSE);
		_children_free_pending.push_back(C);
	}
	else
	{
#ifdef _EDITOR
		Msg("!Can't use looped effect '%s' as 'On Birth' child for group.", nm);
#else
		Debug.fatal(DEBUG_INFO, "Can't use looped effect '%s' as 'On Birth' child for group.", nm);
#endif
	}
}
void CParticleGroup::SItem::Play()
{
	CParticleEffect* PE = static_cast<CParticleEffect*>(_effect);
	if (PE)
		PE->Play();
}
void CParticleGroup::SItem::Stop(BOOL def_stop)
{
	// stop all effects
	CParticleEffect* PE = static_cast<CParticleEffect*>(_effect);
	if (PE)
		PE->Stop(def_stop);
	VisualVecIt it;
	for (it = _children_related.begin(); it != _children_related.end(); it++)
		static_cast<CParticleEffect*>(*it)->Stop(def_stop);
	for (it = _children_free.begin(); it != _children_free.end(); it++)
		static_cast<CParticleEffect*>(*it)->Stop(def_stop);
	// and delete if !deffered
	if (!def_stop)
	{
		for (it = _children_related.begin(); it != _children_related.end(); it++)
			::Render->model_Delete(*it);
		for (it = _children_free.begin(); it != _children_free.end(); it++)
			::Render->model_Delete(*it);
		_children_related.clear();
		_children_free.clear();
	}
}
BOOL CParticleGroup::SItem::IsPlaying()
{
	CParticleEffect* PE = static_cast<CParticleEffect*>(_effect);
	return PE ? PE->IsPlaying() : FALSE;
}
void CParticleGroup::SItem::UpdateParent(const float4x4& m, const float3& velocity, BOOL bTransform)
{
	CParticleEffect* PE = static_cast<CParticleEffect*>(_effect);
	if (PE)
		PE->UpdateParent(m, velocity, bTransform);
}
//------------------------------------------------------------------------------
void OnGroupParticleBirth(void* owner, u32 param, PAPI::Particle& m, u32 idx)
{
	CParticleGroup* PG = static_cast<CParticleGroup*>(owner);
	VERIFY(PG);
	CParticleEffect* PE = static_cast<CParticleEffect*>(PG->items[param]._effect);
	PS::OnEffectParticleBirth(PE, param, m, idx);
	// if have child
	const CPGDef* PGD = PG->GetDefinition();
	VERIFY(PGD);
	const CPGDef::SEffect* eff = PGD->m_Effects[param];
	if (eff->m_Flags.is(CPGDef::SEffect::flOnBirthChild))
		PG->items[param].StartFreeChild(PE, *eff->m_OnBirthChildName, m);
	if (eff->m_Flags.is(CPGDef::SEffect::flOnPlayChild))
		PG->items[param].StartRelatedChild(PE, *eff->m_OnPlayChildName, m);
}
void OnGroupParticleDead(void* owner, u32 param, PAPI::Particle& m, u32 idx)
{
	CParticleGroup* PG = static_cast<CParticleGroup*>(owner);
	VERIFY(PG);
	CParticleEffect* PE = static_cast<CParticleEffect*>(PG->items[param]._effect);
	PS::OnEffectParticleDead(PE, param, m, idx);
	// if have child
	const CPGDef* PGD = PG->GetDefinition();
	VERIFY(PGD);
	const CPGDef::SEffect* eff = PGD->m_Effects[param];
	if (eff->m_Flags.is(CPGDef::SEffect::flOnPlayChild))
		PG->items[param].StopRelatedChild(idx);
	if (eff->m_Flags.is(CPGDef::SEffect::flOnDeadChild))
		PG->items[param].StartFreeChild(PE, *eff->m_OnDeadChildName, m);
}
//------------------------------------------------------------------------------
struct zero_vis_pred
{
	bool operator()(const IRender_Visual* x)
	{
		return x == 0;
	}
};
void CParticleGroup::SItem::OnFrame(u32 u_dt, const CPGDef::SEffect& def, Fbox& box, bool& bPlaying)
{
	CParticleEffect* PE = static_cast<CParticleEffect*>(_effect);
	if (PE)
	{
		PE->OnFrame(u_dt);
		if (PE->IsPlaying())
		{
			bPlaying = true;
			if (PE->vis.box.is_valid())
				box.merge(PE->vis.box);
			if (def.m_Flags.is(CPGDef::SEffect::flOnPlayChild) && def.m_OnPlayChildName.size())
			{
				PAPI::Particle* particles;
				u32 p_cnt;
				PAPI::ParticleManager()->GetParticles(PE->GetHandleEffect(), particles, p_cnt);
				VERIFY(p_cnt == _children_related.size());
				if (p_cnt)
				{
					for (u32 i = 0; i < p_cnt; i++)
					{
						PAPI::Particle& m = particles[i];
						CParticleEffect* C = static_cast<CParticleEffect*>(_children_related[i]);
						float4x4 M;
						M.translate(m.pos);
						float3 vel;
						vel.sub(m.pos, m.posB);
						vel.div(fDT_STEP);
						C->UpdateParent(M, vel, FALSE);
					}
				}
			}
		}
	}
	VisualVecIt it;
	if (!_children_related.empty())
	{
		for (it = _children_related.begin(); it != _children_related.end(); it++)
		{
			CParticleEffect* PEChildrenRelated = static_cast<CParticleEffect*>(*it);
			if (PEChildrenRelated)
			{
				PEChildrenRelated->OnFrame(u_dt);
				if (PEChildrenRelated->IsPlaying())
				{
					bPlaying = true;
					if (PEChildrenRelated->vis.box.is_valid())
						box.merge(PEChildrenRelated->vis.box);
				}
				else
				{
					if (def.m_Flags.is(CPGDef::SEffect::flOnPlayChildRewind))
					{
						PEChildrenRelated->Play();
					}
				}
			}
		}
	}
	if (!_children_free.empty())
	{
		// 1. Делаем копию и очищаем оригинал
		VisualVec local_free = std::move(_children_free);
		_children_free.clear();

		std::vector<IRender_Visual*> to_delete;

		for (IRender_Visual* vis : local_free)
		{
			CParticleEffect* PEChildrenFree = static_cast<CParticleEffect*>(vis);
			if (PEChildrenFree)
			{
				PEChildrenFree->OnFrame(u_dt);

				if (PEChildrenFree->IsPlaying())
				{
					bPlaying = true;
					if (PEChildrenFree->vis.box.is_valid())
						box.merge(PEChildrenFree->vis.box);
					_children_free.push_back(vis);
				}
				else
				{
					to_delete.push_back(vis);
				}
			}
		}

		// Удаляем дубликаты (на случай, если один эффект был добавлен дважды)
		std::sort(to_delete.begin(), to_delete.end());
		to_delete.erase(std::unique(to_delete.begin(), to_delete.end()), to_delete.end());

		// Удаляем остановившиеся эффекты
		for (IRender_Visual* vis : to_delete)
		{
			if (vis)
				::Render->model_Delete(vis);
		}
	}
	if (!_children_free_pending.empty())
	{
		_children_free.insert(_children_free.end(), _children_free_pending.begin(), _children_free_pending.end());
		_children_free_pending.clear();
	}
	//	Msg("C: %d CS: %d",_children.size(),_children_stopped.size());
}
void CParticleGroup::SItem::OnDeviceCreate()
{
	VisualVec visuals;
	GetVisuals(visuals);
	for (VisualVecIt it = visuals.begin(); it != visuals.end(); it++)
		static_cast<CParticleEffect*>(*it)->OnDeviceCreate();
}
void CParticleGroup::SItem::OnDeviceDestroy()
{
	VisualVec visuals;
	GetVisuals(visuals);
	for (VisualVecIt it = visuals.begin(); it != visuals.end(); it++)
		static_cast<CParticleEffect*>(*it)->OnDeviceDestroy();
}
u32 CParticleGroup::SItem::ParticlesCount()
{
	u32 p_count = 0;
	VisualVec visuals;
	GetVisuals(visuals);
	for (VisualVecIt it = visuals.begin(); it != visuals.end(); it++)
		p_count += static_cast<CParticleEffect*>(*it)->ParticlesCount();
	return p_count;
}

//------------------------------------------------------------------------------
// Particle Group part
//------------------------------------------------------------------------------
CParticleGroup::CParticleGroup()
{
	m_RT_Flags.zero();
	m_InitialPosition.set(0, 0, 0);
}

CParticleGroup::~CParticleGroup()
{
	// Msg ("!!! destoy PG");
	for (u32 i = 0; i < items.size(); i++)
		items[i].Clear();
	items.clear();
}

void CParticleGroup::OnFrame(u32 u_dt)
{
	if (m_Def && m_RT_Flags.is(flRT_Playing))
	{
		float ct = m_CurrentTime;
		float f_dt = float(u_dt) / 1000.f;
		for (CPGDef::EffectVec::const_iterator e_it = m_Def->m_Effects.begin(); e_it != m_Def->m_Effects.end(); e_it++)
		{
			if ((*e_it)->m_Flags.is(CPGDef::SEffect::flEnabled))
			{
				VERIFY(items.size() == m_Def->m_Effects.size());
				SItem& I = items[e_it - m_Def->m_Effects.begin()];
				if (I.IsPlaying())
				{
					if ((ct <= (*e_it)->m_Time1) && (ct + f_dt >= (*e_it)->m_Time1))
						I.Stop((*e_it)->m_Flags.is(CPGDef::SEffect::flDefferedStop));
				}
				else
				{
					if (!m_RT_Flags.is(flRT_DefferedStop))
						if ((ct <= (*e_it)->m_Time0) && (ct + f_dt >= (*e_it)->m_Time0))
							I.Play();
				}
			}
		}
		m_CurrentTime += f_dt;
		if ((m_CurrentTime > m_Def->m_fTimeLimit) && (m_Def->m_fTimeLimit > 0.f))
			if (!m_RT_Flags.is(flRT_DefferedStop))
				Stop(true);

		bool bPlaying = false;
		Fbox box;
		box.invalidate();
		for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
			i_it->OnFrame(u_dt, *m_Def->m_Effects[i_it - items.begin()], box, bPlaying);

		if (m_RT_Flags.is(flRT_DefferedStop) && !bPlaying)
		{
			m_RT_Flags.set(flRT_Playing | flRT_DefferedStop, FALSE);
		}
		if (box.is_valid())
		{
			vis.box.set(box);
			vis.box.getsphere(vis.sphere.P, vis.sphere.R);
		}
	}
	else
	{
		vis.box.set(m_InitialPosition, m_InitialPosition);
		vis.box.grow(EPS_L);
		vis.box.getsphere(vis.sphere.P, vis.sphere.R);
	}
}

void CParticleGroup::UpdateParent(const float4x4& m, const float3& velocity, BOOL bTransform)
{
	m_InitialPosition = m.c;
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		i_it->UpdateParent(m, velocity, bTransform);
}

BOOL CParticleGroup::Compile(CPGDef* def)
{
	m_Def = def;
	// destroy existing
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		i_it->Clear();
	items.clear();
	// create new
	if (m_Def)
	{
		items.resize(m_Def->m_Effects.size());
		for (CPGDef::EffectVec::const_iterator e_it = m_Def->m_Effects.begin(); e_it != m_Def->m_Effects.end(); e_it++)
		{
			CParticleEffect* eff = (CParticleEffect*)RenderImplementation.model_CreatePE(*(*e_it)->m_EffectName);
			eff->SetBirthDeadCB(OnGroupParticleBirth, OnGroupParticleDead, this, u32(e_it - m_Def->m_Effects.begin()));
			items[e_it - def->m_Effects.begin()].Set(eff);
		}
	}
	return TRUE;
}

void CParticleGroup::Play()
{
	m_CurrentTime = 0;
	m_RT_Flags.set(flRT_DefferedStop, FALSE);
	m_RT_Flags.set(flRT_Playing, TRUE);
}

void CParticleGroup::Stop(BOOL bDefferedStop)
{
	if (bDefferedStop)
	{
		m_RT_Flags.set(flRT_DefferedStop, TRUE);
	}
	else
	{
		m_RT_Flags.set(flRT_Playing, FALSE);
	}
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		i_it->Stop(bDefferedStop);
}

void CParticleGroup::OnDeviceCreate()
{
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		i_it->OnDeviceCreate();
}

void CParticleGroup::OnDeviceDestroy()
{
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		i_it->OnDeviceDestroy();
}

u32 CParticleGroup::ParticlesCount()
{
	int p_count = 0;
	for (SItemVecIt i_it = items.begin(); i_it != items.end(); i_it++)
		p_count += i_it->ParticlesCount();
	return p_count;
}
