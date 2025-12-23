/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment_geometry_provider.h
====================================================================================================
  Author: NSDeathman & Gemini 3
  Description: Реализация интерфейса IGeometryProvider.
  Этот класс выступает мостом между Presence Audio SDK и физическим движком X-Ray (CDB/ObjectSpace).
====================================================================================================

  Copyright (c) 2025 Nocturning Studio, NSDeathman & Gemini 3

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
	 copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
	 must include attribution to "Presence Audio SDK by Nocturning Studio" in its
	 documentation, credits, or about screen.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

====================================================================================================
  Developed by: NSDeathman (Architecture & Core), Gemini 3 (Optimization & Math)
  Organization: Nocturning Studio
====================================================================================================
*/
#pragma once
#include "stdafx.h"

// Подключение API библиотеки звука
#include <PresenceAudioSDK/PresenceAudioAPI.h>

// Подключение заголовков движка X-Ray
#include "../xrEngine/igame_level.h"
#include "../xrEngine/xr_area.h"
#include "..\xrGame\GameMtlLib.h"

class XRayGeometryAdapter : public Presence::IGeometryProvider
{
  public:
	// Кэш теперь хранит int ID, так как библиотека перешла на int идентификаторы
	xr_vector<int> m_MaterialCache;
	bool m_bCacheBuilt;

	XRayGeometryAdapter() : m_bCacheBuilt(false)
	{
	}

	// ---------------------------------------------------------------------------------------------
	// Настройка параметров материала
	// ---------------------------------------------------------------------------------------------
	// Вспомогательная функция для заполнения структуры параметров
	Presence::MaterialParams GetDefaultParamsForType(Presence::MaterialType type)
	{
		Presence::MaterialParams p;
		// Дефолтные значения (можно тюнить под свой вкус)
		switch (type)
		{
		case Presence::MaterialType::Stone:
			p.transmission = 0.02f;
			p.reflectivity = 0.60f;
			p.absorption = 0.10f;
			p.rt60_weight = 0.8f;
			break;
		case Presence::MaterialType::Metal:
			p.transmission = 0.00f;
			p.reflectivity = 0.85f;
			p.absorption = 0.05f;
			p.rt60_weight = 0.9f;
			break;
		case Presence::MaterialType::Wood:
			p.transmission = 0.08f;
			p.reflectivity = 0.25f;
			p.absorption = 0.30f;
			p.rt60_weight = 0.5f;
			break;
		case Presence::MaterialType::Soft: // Трава, земля, ковры
			p.transmission = 0.40f;
			p.reflectivity = 0.05f;
			p.absorption = 0.90f;
			p.rt60_weight = 0.1f;
			break;
		case Presence::MaterialType::Glass:
			p.transmission = 0.60f;
			p.reflectivity = 0.40f;
			p.absorption = 0.05f;
			p.rt60_weight = 0.2f;
			break;
		case Presence::MaterialType::Absorber:
			p.transmission = 0.01f;
			p.reflectivity = 0.00f;
			p.absorption = 1.00f;
			p.rt60_weight = 0.0f;
			break;
		default: // Air / Default
			p.transmission = 0.99f;
			p.reflectivity = 0.00f;
			p.absorption = 0.00f;
			p.rt60_weight = 0.0f;
			break;
		}
		return p;
	}

	// ---------------------------------------------------------------------------------------------
	// Построение кэша
	// ---------------------------------------------------------------------------------------------
	// Теперь принимаем указатель на систему, чтобы зарегистрировать свойства материалов!
	void BuildMaterialCache(Presence::AudioSystem* pSystem)
	{
		if (m_bCacheBuilt || !pSystem)
			return;

		Msg("[Presence EAX] Building material cache and registering physics...");

		// 1. Сначала настроим базовые пресеты в самой библиотеке
		// Мы мапим enum class на int ID
		for (int i = 0; i < (int)Presence::MaterialType::Count; i++)
		{
			Presence::MaterialType type = (Presence::MaterialType)i;
			pSystem->SetMaterialProperties(i, GetDefaultParamsForType(type));
		}

		// 2. Теперь проходимся по материалам X-Ray и линкуем их к ID библиотеки
		u32 mtlCount = GMLib.CountMaterial();
		m_MaterialCache.reserve(mtlCount);

		for (u32 i = 0; i < mtlCount; i++)
		{
			SGameMtl* mtl = GMLib.GetMaterialByIdx(i);
			Presence::MaterialType mappedType = Presence::MaterialType::Stone;

			if (mtl)
			{
				LPCSTR name = mtl->m_Name.c_str();

				if (strstr(name, "fake") || strstr(name, "setka_rabica"))
					mappedType = Presence::MaterialType::Air;
				else if (strstr(name, "wood") || strstr(name, "trees") || strstr(name, "plank"))
					mappedType = Presence::MaterialType::Wood;
				else if (strstr(name, "metal") || strstr(name, "grate") || strstr(name, "tin") ||
						 strstr(name, "pipe") || strstr(name, "door"))
					mappedType = Presence::MaterialType::Metal;
				else if (strstr(name, "glass") || strstr(name, "ice") || strstr(name, "window"))
					mappedType = Presence::MaterialType::Glass;
				else if (strstr(name, "earth") || strstr(name, "grass") || strstr(name, "bush") ||
						 strstr(name, "water") || strstr(name, "cloth") || strstr(name, "fabric"))
					mappedType = Presence::MaterialType::Soft;
				else if (strstr(name, "absorber") || strstr(name, "foam") || strstr(name, "padding"))
					mappedType = Presence::MaterialType::Absorber;
				else
					mappedType = Presence::MaterialType::Stone; // Бетон/Кирпич по умолчанию
			}

			// Сохраняем ID (cast enum to int)
			m_MaterialCache.push_back((int)mappedType);
		}

		m_bCacheBuilt = true;
		Msg("[Presence EAX] Material cache built. Total mapped materials: %d", m_MaterialCache.size());
	}

	// ---------------------------------------------------------------------------------------------
	// Ray Casting
	// ---------------------------------------------------------------------------------------------
	virtual Presence::RayHit CastRay(const Presence::float3& start, const Presence::float3& dir, float maxDist) override
	{
		// Примечание: BuildMaterialCache должен быть вызван ДО первого CastRay из CSoundEnvironment

		Presence::RayHit result;
		result.isHit = false;
		result.distance = maxDist;
		result.materialID = 0; // 0 = Air по умолчанию

		if (!g_pGameLevel)
			return result;

		Fvector xStart, xDir;
		xStart.set(start.x, start.y, start.z);
		xDir.set(dir.x, dir.y, dir.z);

		// Нормализация обязательна для движка X-Ray
		float dirLen = xDir.magnitude();
		if (dirLen > 0.0001f)
			xDir.div(dirLen);
		else
			return result;

		const float K_BIAS = 0.05f;
		xStart.mad(xDir, K_BIAS);

		float traceDist = maxDist - K_BIAS;
		if (traceDist <= 0.001f)
			return result;

		collide::rq_result rq;

		// RayPick по статике
		BOOL hit = g_pGameLevel->ObjectSpace.RayPick(xStart, xDir, traceDist, collide::rqtStatic, rq, NULL);

		if (hit)
		{
			result.isHit = true;
			result.distance = rq.range + K_BIAS;

			CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + rq.element;
			Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

			Fvector xNorm;
			xNorm.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

			// ВАЖНО: Присваиваем нормаль и ID материала
			result.normal = Presence::float3(xNorm.x, xNorm.y, xNorm.z);

			u16 mtl_idx = (u16)tri->material;
			if (mtl_idx < m_MaterialCache.size())
				result.materialID = m_MaterialCache[mtl_idx];
			else
				result.materialID = (int)Presence::MaterialType::Stone;
		}

		return result;
	}
};
