#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "Environment_wind.h"

CEnvWind::CEnvWind()
{
	m_wind_strength = 0.f;
	m_wind_direction = 0.f;
	m_wind_gusting = 0.f;
}

CEnvWind::~CEnvWind()
{
}

void CEnvWind::load(CInifile& config, const shared_str& section)
{
	m_load_section = section;

	m_wind_strength = config.r_float(m_load_section, "wind_strength");
	m_wind_direction = deg2rad(config.r_float(m_load_section, "wind_direction"));
	m_wind_gusting = config.r_float(m_load_section, "wind_gusting");
	m_wind_tilt = config.r_float(m_load_section, "wind_tilt") * (PI / 180.0f); // Конвертируем в радианы
	m_wind_velocity = config.r_float(m_load_section, "wind_velocity");
}

CEnvWind* CEnvironment::AppendEnvWind(const shared_str& sect)
{
	// 1. Ищем, загружен ли уже такой пресет
	for (EnvWindVecIt it = Winds.begin(); it != Winds.end(); it++)
		if ((*it)->name().equal(sect))
			return (*it);

	// 2. Если нет, проверяем наличие секции в файле winds.ltx
	if (!m_winds_config->section_exist(sect))
	{
		Msg("! Error: Wind section '%s' not found in environment\\winds.ltx", sect.c_str());
		return NULL;
	}

	// 3. Создаем, загружаем и добавляем в список
	Winds.push_back(xr_new<CEnvWind>());
	Winds.back()->load(*m_winds_config, sect);
	return (Winds.back());
}

// Генерирует плавную волну порывов ветра (0.0 ... 1.0)
float CalcGustWave(float Time)
{
	// Максимально простая и плавная волна.
	// Используем всего две близкие частоты.
	// Это создаст эффект "биения" (медленного нарастания и спада), без резких скачков.

	float wave1 = sin(Time * 0.05f);		// Основной цикл (медленный)
	float wave2 = sin(Time * 0.07f) * 0.5f; // Модуляция

	float result = wave1 + wave2;

	// Мягкая нормализация в 0..1
	// Используем (sin + 1) / 2, чтобы избежать острых углов
	return (result / 3.0f) + 0.5f;
}

void CEnvironment::CalcWindValues()
{
	float GustFactor = CalcGustWave(Engine.TimeManager.GetGlobalTime());

	float BaseStrength = CurrentEnv->wind_strength;
	// Уменьшили влияние порывов на итоговую цифру, чтобы не было скачков от 0 до 100
	float GustStrength = CurrentEnv->wind_gusting * BaseStrength;

	CurrentEnv->wind_turbulence = BaseStrength + (GustStrength * GustFactor);

	clamp(CurrentEnv->wind_turbulence, 0.0f, 5.0f);

	float yaw = CurrentEnv->wind_direction;
	float pitch = CurrentEnv->wind_tilt;

	// X-Ray использует Y-Up систему координат
	CurrentEnv->wind_direction3D.x = _cos(yaw) * _cos(pitch);
	CurrentEnv->wind_direction3D.y = _sin(pitch);
	CurrentEnv->wind_direction3D.z = _sin(yaw) * _cos(pitch);
}