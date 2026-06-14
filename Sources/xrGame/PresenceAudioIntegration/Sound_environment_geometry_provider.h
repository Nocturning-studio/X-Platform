/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment_geometry_provider.h
====================================================================================================
*/
#pragma once
#include "stdafx.h"
#include <PresenceAudioSDK/PresenceAudioAPI.h>
#include "../xrEngine/igame_level.h"
#include "../xrEngine/xr_area.h"
#include "..\xrGame\GameMtlLib.h"

class XRayGeometryAdapter : public Presence::IGeometryProvider
{
  public:
	xr_vector<int> m_MaterialCache;
	bool m_bCacheBuilt;

	XRayGeometryAdapter() : m_bCacheBuilt(false)
	{
	}

	// -------------------------------------------------------------------------
	// Настройка дефолтных пресетов
	// -------------------------------------------------------------------------
	Presence::MaterialParams GetDefaultParams(Presence::MaterialType type)
	{
		Presence::MaterialParams p;
		switch (type)
		{
		case Presence::MaterialType::Stone:
			return {0.05f, 0.60f, 0.10f, 0.8f};
		case Presence::MaterialType::Metal:
			return {0.00f, 0.85f, 0.05f, 0.9f};
		case Presence::MaterialType::Wood:
			return {0.15f, 0.25f, 0.30f, 0.5f};
		case Presence::MaterialType::Soft:
			return {0.50f, 0.05f, 0.90f, 0.1f};
		case Presence::MaterialType::Glass:
			return {0.70f, 0.40f, 0.05f, 0.2f};
		case Presence::MaterialType::Absorber:
			return {0.01f, 0.00f, 1.00f, 0.0f};
		default:
			return {0.99f, 0.00f, 0.00f, 0.0f}; // Air
		}
	}

	// -------------------------------------------------------------------------
	// Построение кэша
	// -------------------------------------------------------------------------
	void BuildMaterialCache(Presence::AudioSystem* pSystem)
	{
		if (!pSystem)
			return;
		// Если кэш перестраивается (hot reload), очищаем старый
		m_MaterialCache.clear();

		Msg("[Presence Audio] Building material cache...");

		// 1. Регистрируем базовые типы
		for (int i = 0; i < (int)Presence::MaterialType::Count; i++)
		{
			pSystem->SetMaterialProperties(i, GetDefaultParams((Presence::MaterialType)i));
		}

		// 2. Читаем конфиг LTX
		string_path configPath;
		FS.update_path(configPath, "$game_config$", "presence_audio_materials.ltx");
		CInifile* pConfig = FS.exist(configPath) ? new CInifile(configPath, TRUE, TRUE, FALSE) : nullptr;

		if (pConfig)
			Msg("[Presence Audio] Loaded config: %s", configPath);
		else
			Msg("! [Presence Audio] Config not found, using heuristics.");

		// 3. Маппинг материалов движка
		u32 mtlCount = GMLib.CountMaterial();
		m_MaterialCache.resize(mtlCount);

		int customCount = 0;

		for (u32 i = 0; i < mtlCount; i++)
		{
			SGameMtl* mtl = GMLib.GetMaterialByIdx(i);
			if (!mtl)
			{
				m_MaterialCache[i] = (int)Presence::MaterialType::Stone;
				continue;
			}

			LPCSTR name = mtl->m_Name.c_str();
			int finalID = (int)Presence::MaterialType::Stone;

			// A. Проверка конфига
			if (pConfig && pConfig->section_exist(name))
			{
				Presence::MaterialParams p;
				p.transmission = pConfig->r_float(name, "transmission");
				p.reflectivity = pConfig->r_float(name, "reflectivity");
				p.absorption = pConfig->r_float(name, "absorption");
				p.rt60_weight =
					pConfig->line_exist(name, "rt60_weight") ? pConfig->r_float(name, "rt60_weight") : p.reflectivity;

				finalID = pSystem->CreateCustomMaterial(p);
				customCount++;
			}
			// B. Эвристика
			else
			{
				// Специальная проверка для невидимых стен/сеток (важно для геймплея!)
				if (strstr(name, "fake") || strstr(name, "invisible") || strstr(name, "setka"))
					finalID = (int)Presence::MaterialType::Air;
				else if (strstr(name, "wood") || strstr(name, "plank"))
					finalID = (int)Presence::MaterialType::Wood;
				else if (strstr(name, "metal") || strstr(name, "pipe") || strstr(name, "door"))
					finalID = (int)Presence::MaterialType::Metal;
				else if (strstr(name, "glass") || strstr(name, "window"))
					finalID = (int)Presence::MaterialType::Glass;
				else if (strstr(name, "grass") || strstr(name, "earth") || strstr(name, "cloth"))
					finalID = (int)Presence::MaterialType::Soft;
				else if (strstr(name, "asphalt") || strstr(name, "concrete"))
					finalID = (int)Presence::MaterialType::Stone;
			}
			m_MaterialCache[i] = finalID;
		}

		if (pConfig)
			xr_delete(pConfig);
		m_bCacheBuilt = true;
		Msg("[Presence Audio] Cache built: %d materials (%d custom).", m_MaterialCache.size(), customCount);
	}

	// -------------------------------------------------------------------------
	// Трассировка (Выполняется в рабочем потоке SDK!)
	// Внимание: доступ к ObjectSpace.RayPick должен быть thread-safe.
	// В X-Ray чтение статической геометрии обычно безопасно, если уровень загружен.
	// -------------------------------------------------------------------------
	virtual Presence::RayHit CastRay(const Presence::float3& start, const Presence::float3& dir, float maxDist) override
	{
		Presence::RayHit result;
		result.isHit = false;
		result.distance = maxDist;
		result.materialID = 0;

		// Критическая проверка: уровень мог выгрузиться
		if (!g_pGameLevel)
			return result;

		// Конвертация векторов
		fvec3 xStart, xDir;
		xStart.set(start.x, start.y, start.z);
		xDir.set(dir.x, dir.y, dir.z);

		// Валидация направления (предотвращение NaN)
		float mag = xDir.magnitude();
		if (mag < EPS_S)
			return result;
		xDir.div(mag);

		// Смещение луча для избежания самопересечения
		const float K_BIAS = 0.05f;
		xStart.mad(xDir, K_BIAS);
		float traceDist = maxDist - K_BIAS;
		if (traceDist <= EPS_S)
			return result;

		collide::rq_result rq;

		// Трассировка только по статике (rqtStatic) для скорости и стабильности
		// Игнорируем динамические объекты (NPC, ящики) для расчета реверберации
		BOOL hit = g_pGameLevel->ObjectSpace.RayPick(xStart, xDir, traceDist, collide::rqtStatic, rq, NULL);

		if (hit)
		{
			result.isHit = true;
			result.distance = rq.range + K_BIAS;

			// Получение треугольника для нормали и материала
			CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + rq.element;
			fvec3* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

			fvec3 xNorm;
			xNorm.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);
			result.normal = Presence::float3(xNorm.x, xNorm.y, xNorm.z);

			u16 mtl_idx = (u16)tri->material;
			if (mtl_idx < m_MaterialCache.size())
				result.materialID = m_MaterialCache[mtl_idx];
			else
				result.materialID = (int)Presence::MaterialType::Stone; // Fallback
		}

		return result;
	}
};
