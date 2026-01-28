#include "stdafx.h"
#include "Light_Package.h"

void light_Package::clear()
{
	v_point.clear();
	v_spot.clear();
	v_shadowed.clear();
}

IC bool pred_light_cmp(light* _1, light* _2)
{
	if (_1->m_VisibilityData.pending)
	{
		if (_2->m_VisibilityData.pending)
			return _1->m_VisibilityData.query_order > _2->m_VisibilityData.query_order; // q-order
		else
			return false; // _2 should be first
	}
	else
	{
		if (_2->m_VisibilityData.pending)
			return true; // _1 should be first
		else
			return _1->get_range() > _2->get_range(); // sort by range
	}
}

void light_Package::sort()
{
	// resort lights (pending -> at the end), maintain stable order
	std::stable_sort(v_point.begin(), v_point.end(), pred_light_cmp);
	std::stable_sort(v_spot.begin(), v_spot.end(), pred_light_cmp);
	std::stable_sort(v_shadowed.begin(), v_shadowed.end(), pred_light_cmp);
}

