#include "stdafx.h"
#include "radioactivezone.h"
#include "hudmanager.h"
#include "level.h"
#include "xrmessages.h"
#include "../xrEngine/bone.h"
#include "clsid_game.h"
#include "game_base_space.h"
#include "Hit.h"

CRadioactiveZone::CRadioactiveZone(void)
{
}

CRadioactiveZone::~CRadioactiveZone(void)
{
}

void CRadioactiveZone::Load(LPCSTR section)
{
	inherited::Load(section);
}

bool CRadioactiveZone::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if (!result)
		UpdateBlowout();
	return result;
}

void CRadioactiveZone::Affect(SZoneObjectInfo* O)
{
	// вермя срабатывания не чаще, чем заданный период
	if (m_dwDeltaTime < m_dwPeriod)
		return;
	//.	m_dwDeltaTime = 0;

	CGameObject* GO = O->object;

	if (GO)
	{
		float3 pos;
		Transform().transform_tiny(pos, CFORM()->getSphere().P);

#ifdef DEBUG
		char pow[255];
		sprintf_s(pow, "zone hit. %.3f", Power(GO->Position().distance_to(pos)));
		if (bDebug)
			Msg("%s %s", *GO->cName(), pow);
#endif

		float3 dir;
		dir.set(0, 0, 0);

		float3 position_in_bone_space;
		float power = (GameID() == GAME_SINGLE) ? Power(GO->Position().distance_to(pos)) : 0.0f;
		float impulse = 0.f;
		if (power > EPS)
		{
			//.			m_dwDeltaTime = 0;
			position_in_bone_space.set(0.f, 0.f, 0.f);

			CreateHit(GO->ID(), ID(), dir, power, BI_NONE, position_in_bone_space, impulse, ALife::eHitTypeRadiation);
		}
	}
}

void CRadioactiveZone::feel_touch_new(CObject* O)
{
	inherited::feel_touch_new(O);
	if (GameID() != GAME_SINGLE)
	{
		if (O->CLS_ID == CLSID_OBJECT_ACTOR)
		{
			CreateHit(O->ID(), ID(), float3().set(0, 0, 0), 0.0f, BI_NONE, float3().set(0, 0, 0), 0.0f,
					  ALife::eHitTypeRadiation);
		}
	};
};

#include "actor.h"
BOOL CRadioactiveZone::feel_touch_contact(CObject* O)
{
	CActor* A = smart_cast<CActor*>(O);
	if (A)
	{
		// Дополнительная проверка на валидность объекта
		if (A->getDestroy())
			return FALSE;

		// "Failsafe": Проверка по дистанции.
		// Если физика залагала и Contact() врет, мы проверяем математическое расстояние.
		// Берем радиус ограничивающей сферы (bounding sphere) зоны.
		float fZoneRadius = CFORM()->getSphere().R;
		float fDist = A->Position().distance_to(Position());

		// Если мы дальше радиуса зоны + 2.5 метра (запас на гистерезис),
		// то принудительно считаем, что контакта нет.
		if (fDist > (fZoneRadius + 2.5f))
			return FALSE;

		if (!((CCF_Shape*)CFORM())->Contact(O))
			return FALSE;

		return A->feel_touch_on_contact(this);
	}
	else
		return FALSE;
}

void CRadioactiveZone::UpdateWorkload(u32 dt)
{
	if (IsEnabled() && GameID() != GAME_SINGLE)
	{
		OBJECT_INFO_VEC_IT it;
		float3 pos;
		Transform().transform_tiny(pos, CFORM()->getSphere().P);
		for (it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it)
		{
			if (!(*it).object->getDestroy() && (*it).object->CLS_ID == CLSID_OBJECT_ACTOR)
			{
				//=====================================
				NET_Packet l_P;
				l_P.write_start();
				l_P.read_start();

				float dist = (*it).object->Position().distance_to(pos);
				float power = Power(dist) * dt / 1000;
				///				Msg("Zone Dist %f, Radiation Power %f, ", dist, power);

				SHit HS;
				HS.GenHeader(GE_HIT, (*it).object->ID());
				HS.whoID = ID();
				HS.weaponID = ID();
				HS.dir = float3().set(0, 0, 0);
				HS.power = power;
				HS.boneID = BI_NONE;
				HS.p_in_bone_space = float3().set(0, 0, 0);
				HS.impulse = 0.0f;
				HS.hit_type = ALife::eHitTypeRadiation;

				HS.Write_Packet_Cont(l_P);

				(*it).object->OnEvent(l_P, HS.PACKET_TYPE);
				//=====================================
			};
		}
	}
	inherited::UpdateWorkload(dt);
}
