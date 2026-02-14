////////////////////////////////////////////////////////////////////////////
//	Module 		: detail_path_manager_space.h
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Detail path manager space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace DetailPathManager
{
enum EDetailPathType
{
	eDetailPathTypeSmooth,
	eDetailPathTypeSmoothDodge,
	eDetailPathTypeSmoothCriteria,
};

struct STravelPathPoint
{
	float3 position;
	u32 vertex_id;
	u32 velocity;

	IC void set_position(const float3& pos)
	{
		position = pos;
	}

	IC void set_vertex_id(const u32 _vertex_id)
	{
		vertex_id = _vertex_id;
	}

	IC float3& get_position()
	{
		return (position);
	}

	IC u32 get_vertex_id()
	{
		return (vertex_id);
	}
};
}; // namespace DetailPathManager
