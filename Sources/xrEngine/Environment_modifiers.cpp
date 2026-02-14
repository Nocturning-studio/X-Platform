//-----------------------------------------------------------------------------
// Environment modifier
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "../xrGame/LevelGameDef.h"

bool CEnvModifier::load(IReader* fs)
{
	fs->r_fvector3(position);
	radius = fs->r_float();
	power = fs->r_float();
	far_plane = fs->r_float();
	fs->r_fvector3(fog_color);
	fog_density = fs->r_float();
	fs->r_fvector3(ambient);
	fs->r_fvector3(sky_color);
	fs->r_fvector3(hemi_color);

	return true;
}

float CEnvModifier::sum(CEnvModifier& M, float3& view)
{
	float _dist_sq = view.distance_to_sqr(M.position);
	if (_dist_sq >= (M.radius * M.radius))
		return 0;

	float _att = 1 - _sqrt(_dist_sq) / M.radius; //[0..1];
	float _power = M.power * _att;

	if (M.use_flags.test(eViewDist))
	{
		far_plane += M.far_plane * _power;
		use_flags.set(eViewDist, TRUE);
	}
	if (M.use_flags.test(eFogColor))
	{
		fog_color.mad(M.fog_color, _power);
		use_flags.set(eFogColor, TRUE);
	}
	if (M.use_flags.test(eFogDensity))
	{
		fog_density += M.fog_density * _power;
		use_flags.set(eFogDensity, TRUE);
	}
	if (M.use_flags.test(eAmbientColor))
	{
		ambient.mad(M.ambient, _power);
		use_flags.set(eAmbientColor, TRUE);
	}
	if (M.use_flags.test(eSkyColor))
	{
		sky_color.mad(M.sky_color, _power);
		use_flags.set(eSkyColor, TRUE);
	}
	if (M.use_flags.test(eHemiColor))
	{
		hemi_color.mad(M.hemi_color, _power);
		use_flags.set(eHemiColor, TRUE);
	}

	return _power;
}