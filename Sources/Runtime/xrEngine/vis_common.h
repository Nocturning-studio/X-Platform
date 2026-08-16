#ifndef vis_commonH
#define vis_commonH
#pragma once

#pragma pack(push, 4)
#include <atomic>

struct vis_data
{
	Fsphere sphere;						 //
	Fbox box;							 //
	u32 m_traversal_marker;				 // for different sub-renders
	u32 accept_frame;					 // when it was requisted accepted for main render
	u32 hom_frame;						 // when to perform test - shedule
	u32 hom_tested;						 // when it was last time tested

	IC void clear()
	{
		sphere.P.set(0, 0, 0);
		sphere.R = 0;
		box.invalidate();
		m_traversal_marker = 0;
		accept_frame = 0;
		hom_frame = 0;
		hom_tested = 0;
	}

	vis_data()
	{
		box.invalidate();
		sphere.P.set(0, 0, 0);
		sphere.R = 0;
		m_traversal_marker = 0;
		accept_frame = 0;
		hom_frame = 0;
		hom_tested = 0;
	}

	vis_data& operator=(const vis_data& other)
	{
		if (this != &other)
		{
			box = other.box;
			sphere = other.sphere;
			accept_frame = other.accept_frame;
			hom_frame = other.hom_frame;
			hom_tested = other.hom_tested;

			m_traversal_marker = other.m_traversal_marker;
		}
		return *this;
	}

	vis_data(const vis_data& other)
	{
		*this = other;
	}
};
#pragma pack(pop)
#endif
