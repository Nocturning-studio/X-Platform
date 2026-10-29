#pragma once

#include "light.h"

class light;

#define DU_SPHERE_NUMVERTEX 92
#define DU_SPHERE_NUMFACES 180
#define DU_SPHERE_PART_NUMVERTEX 82
#define DU_SPHERE_PART_NUMFACES 160
#define DU_CONE_NUMVERTEX 18
#define DU_CONE_NUMFACES 32

class light_Package
{
  public:
	xr_vector<light*> v_point;
	xr_vector<light*> v_spot;
	xr_vector<light*> v_shadowed;

  public:
	void clear();
	void sort();
};
