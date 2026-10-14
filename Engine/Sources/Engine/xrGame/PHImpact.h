#ifndef PH_IMPACT_H
#define PH_IMPACT_H

struct SPHImpact
{
	fvec3 force;
	fvec3 point;
	u16 geom;
	SPHImpact(const fvec3& aforce, const fvec3& apoint, u16 root_geom)
	{
		force.set(aforce);
		point.set(apoint);
		geom = root_geom;
	}
};
DEFINE_VECTOR(SPHImpact, PH_IMPACT_STORAGE, PH_IMPACT_I)

#endif
