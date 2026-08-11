#pragma once

class ENGINE_API CEnvWind
{
  protected:
	shared_str m_load_section;

  public:
	float m_wind_strength;
	float m_wind_direction;
	float m_wind_gusting;
	float m_wind_tilt; // ”гол наклона (в градусах, положительный = вверх, отрицательный = вниз)
	float m_wind_velocity; // —корость прохождени€ волны ветра

  public:
	CEnvWind();
	~CEnvWind();

	void load(CInifile& config, const shared_str& section);

	IC const shared_str& name()
	{
		return m_load_section;
	}
};
