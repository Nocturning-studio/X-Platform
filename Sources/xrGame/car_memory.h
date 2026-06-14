////////////////////////////////////////////////////////////////////////////
//	Module 		: car_memory.h
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : car memory
////////////////////////////////////////////////////////////////////////////

#ifndef CAR_MEMORY_H
#define CAR_MEMORY_H

#include "vision_client.h"

class CCar;

class car_memory : public vision_client
{
  private:
	typedef vision_client inherited;

  private:
	CCar* m_object;
	float m_fov_deg;
	float m_aspect;
	float m_far_plane;
	fvec3 m_view_position;
	fvec3 m_view_direction;
	fvec3 m_view_normal;

  public:
	car_memory(CCar* object);

	virtual void reload(LPCSTR section);

	virtual BOOL feel_vision_isRelevant(CObject* object);
	virtual void camera(fvec3& position, fvec3& direction, fvec3& normal, float& field_of_view,
						float& aspect_ratio, float& near_plane, float& far_plane);
	void set_camera(const fvec3& position, const fvec3& direction, const fvec3& normal);
};

#endif // CAR_MEMORY_H
