#include "stdafx.h"
#include "poltergeist.h"
#include "../../../PhysicsShell.h"
#include "../../../level.h"
#include "../../../material_manager.h"
#include "../../../level_debug.h"

CPolterSpecialAbility::CPolterSpecialAbility(CPoltergeist* polter)
{
	m_object = polter;

	m_particles_object = 0;
	m_particles_object_electro = 0;
}

CPolterSpecialAbility::~CPolterSpecialAbility()
{
	CParticlesObject::Destroy(m_particles_object);
	CParticlesObject::Destroy(m_particles_object_electro);
}

void CPolterSpecialAbility::load(LPCSTR section)
{
	m_particles_hidden = pSettings->r_string(section, "Particles_Hidden");
	m_particles_damage = pSettings->r_string(section, "Particles_Damage");
	m_particles_death = pSettings->r_string(section, "Particles_Death");
	m_particles_idle = pSettings->r_string(section, "Particles_Idle");

	m_sound_base.create(pSettings->r_string(section, "Sound_Idle"), st_Effect, SOUND_TYPE_MONSTER_TALKING);

	m_last_hit_frame = 0;
}

void CPolterSpecialAbility::update_schedule()
{
	if (m_object->g_Alive())
	{
		if (!m_sound_base._feedback())
			m_sound_base.play_at_pos(m_object, m_object->Position());
		else
			m_sound_base.set_position(m_object->Position());
	}
}

void CPolterSpecialAbility::on_hide()
{
	VERIFY(m_particles_object == 0);
	if (!m_object->g_Alive())
		return;
	m_particles_object =
		m_object->PlayParticles(m_particles_hidden, m_object->Position(), float3().set(0.0f, 0.1f, 0.0f), false);
	m_particles_object_electro =
		m_object->PlayParticles(m_particles_idle, m_object->Position(), float3().set(0.0f, 0.1f, 0.0f), false);
}

void CPolterSpecialAbility::on_show()
{
	if (m_particles_object)
		CParticlesObject::Destroy(m_particles_object);
	if (m_particles_object_electro)
		CParticlesObject::Destroy(m_particles_object_electro);
}

void CPolterSpecialAbility::update_frame()
{
	if (m_particles_object)
		m_particles_object->SetTransform(m_object->Transform());
	if (m_particles_object_electro)
		m_particles_object_electro->SetTransform(m_object->Transform());
}

void CPolterSpecialAbility::on_die()
{
	float3 particles_position = m_object->m_current_position;
	particles_position.y += m_object->target_height;

	m_object->PlayParticles(m_particles_death, particles_position, float3().set(0.0f, 1.0f, 0.0f), TRUE, FALSE);

	CParticlesObject::Destroy(m_particles_object_electro);
	CParticlesObject::Destroy(m_particles_object);
}

void CPolterSpecialAbility::on_hit(SHit* pHDS)
{
	if (m_object->g_Alive() && (pHDS->hit_type == ALife::eHitTypeFireWound) && (Engine.TimeManager.GetFrameCount() != m_last_hit_frame))
	{
		if (BI_NONE != pHDS->bone())
		{

			// вычислить координаты попадания
			CKinematics* V = smart_cast<CKinematics*>(m_object->Visual());

			float3 start_pos = pHDS->bone_space_position();
			float4x4& m_bone = V->LL_GetBoneInstance(pHDS->bone()).mTransform;
			m_bone.transform_tiny(start_pos);
			m_object->Transform().transform_tiny(start_pos);

			m_object->PlayParticles(m_particles_damage, start_pos, float3().set(0.f, 1.f, 0.f));
		}
	}

	m_last_hit_frame = Engine.TimeManager.GetFrameCount();
}

//////////////////////////////////////////////////////////////////////////
// Other
//////////////////////////////////////////////////////////////////////////

#define IMPULSE 10.f
#define IMPULSE_RADIUS 5.f
#define TRACE_DISTANCE 10.f
#define TRACE_ATTEMPT_COUNT 3

void CPoltergeist::PhysicalImpulse(const float3& position)
{
	m_nearest.clear_not_free();
	Level().ObjectSpace.GetNearest(m_nearest, position, IMPULSE_RADIUS, NULL);
	// xr_vector<CObject*> &m_nearest = Level().ObjectSpace.q_nearest;
	if (m_nearest.empty())
		return;

	u32 index = Random.randI(m_nearest.size());

	CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(m_nearest[index]);
	if (!obj || !obj->m_pPhysicsShell)
		return;

	float3 dir;
	dir.sub(obj->Position(), position);
	dir.normalize();

	CPhysicsElement* E =
		obj->m_pPhysicsShell->get_ElementByStoreOrder(u16(Random.randI(obj->m_pPhysicsShell->get_ElementsNumber())));
	// E->applyImpulse(dir,IMPULSE * obj->m_pPhysicsShell->getMass());
	E->applyImpulse(dir, IMPULSE * E->getMass());
}

void CPoltergeist::StrangeSounds(const float3& position)
{
	if (m_strange_sound._feedback())
		return;

	for (u32 i = 0; i < TRACE_ATTEMPT_COUNT; i++)
	{
		float3 dir;
		dir.random_dir();

		collide::rq_result l_rq;
		if (Level().ObjectSpace.RayPick(position, dir, TRACE_DISTANCE, collide::rqtStatic, l_rq, NULL))
		{
			if (l_rq.range < TRACE_DISTANCE)
			{

				// Получить пару материалов
				CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + l_rq.element;
				SGameMtlPair* mtl_pair = GMLib.GetMaterialPair(material().self_material_idx(), pTri->material);
				if (!mtl_pair)
					continue;

				// Играть звук
				if (!mtl_pair->CollideSounds.empty())
				{
					CLONE_MTL_SOUND(m_strange_sound, mtl_pair, CollideSounds);
					float3 pos;
					pos.mad(position, dir, ((l_rq.range - 0.1f > 0) ? l_rq.range - 0.1f : l_rq.range));
					m_strange_sound.play_at_pos(this, pos);
					return;
				}
			}
		}
	}
}
