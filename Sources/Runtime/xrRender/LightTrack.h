#pragma once

const int lt_hemisamples = 26;

class CROS_impl : public IRender_ObjectSpecific
{
  public:
	enum CubeFaces
	{
		CUBE_FACE_POS_X,
		CUBE_FACE_POS_Y,
		CUBE_FACE_POS_Z,
		CUBE_FACE_NEG_X,
		CUBE_FACE_NEG_Y,
		CUBE_FACE_NEG_Z,
		NUM_FACES
	};

  public:
	CROS_impl();
	virtual void update(IRenderable* O);
	virtual void update_smooth(IRenderable* O = 0);

	virtual float get_hemi();
	virtual const float* get_hemi_cube();

	// Интерфейс IRender_ObjectSpecific
	virtual void force_mode(u32 mode)
	{
		MODE = mode;
	}
	virtual float get_luminocity()
	{
		return 0.5f;
	} // Заглушка
	virtual float get_luminocity_hemi()
	{
		return get_hemi();
	}
	virtual float* get_luminocity_hemi_cube()
	{
		return hemi_cube_smooth;
	}

  private:
	void smart_update(IRenderable* O);
	void calc_sky_hemi_value(fvec3& position, CObject* _object);
	static inline void accum_hemi(float* hemi_cube, fvec3& dir, float scale);

  private:
	// Состояние трассировки
	bool result[lt_hemisamples];
	collide::ray_cache cache[lt_hemisamples];

	// Текущие значения AO
	float hemi_value;
	float hemi_smooth;
	float hemi_cube[NUM_FACES];
	float hemi_cube_smooth[NUM_FACES];

	// Управление обновлением
	u32 dwFrame;
	u32 dwFrameSmooth;
	fvec3 last_position;
	s32 ticks_to_update;
	s32 sky_rays_uptodate;

	// Счетчики сэмплов
	s32 result_count;
	u32 result_iterator;
	u32 result_frame;

	// Режим (для совместимости)
	u32 MODE;
};
