#pragma once

#include "..\xrEngine\ispatial.h"
#include "light_package.h"
#include "light_smapvis.h"

const u32 delay_small_min = 1;
const u32 delay_small_max = 3;
const u32 delay_large_min = 10;
const u32 delay_large_max = 20;
const u32 cullfragments = 4;

class light : public IRender_Light, public ISpatial
{
  private:
	fvec3 position;
	fvec3 direction;
	fvec3 right;
	float range;
	float cone;
	Fcolor color;

	vis_data hom;
	u32 frame_render;

	xr_vector<IRender_Sector*> m_sectors;

	light* omnipart[6];

	smapvis svis; // used for 6-cubemap faces

	ref_shader s_spot;
	ref_shader s_point;

	u32 m_transform_frame;
	fmat4x4 m_transform;

  public:
	struct _VisibilityData
	{
		u32 frame2test;	 // frame the test is sheduled to
		u32 query_id;	 // ID of occlusion query
		u32 query_order; // order of occlusion query
		bool visible;	 // visible/invisible
		bool pending;	 // test is still pending
		u16 smap_ID;
	} VisibilityData;

	union _TransformContext {
		struct _Sun
		{
			fmat4x4 combine;
			s32 minX, maxX;
			s32 minY, maxY;
			BOOL transluent;
		} Sun;
		struct _ShadowContext
		{
			fmat4x4 view;
			fmat4x4 project;
			fmat4x4 combine;
			u32 size;
			u32 posX;
			u32 posY;
			BOOL transluent;
		} ShadowContext;
	} TransformContext;

	struct _LightFlags
	{
		u32 type : 4;
		u32 bStatic : 1;
		u32 bActive : 1;
		u32 bShadow : 1;
	} LightFlags;

  public:
	fvec3 get_position(){return position;}
	fvec3 get_direction(){return direction;}
	fvec3 get_right(){return right;}
	float get_range(){return range;}
	float get_cone(){return cone;}
	Fcolor get_color(){return color;}
	u32 get_frame_render(){return frame_render;}
	ref_shader get_shader_spot(){return s_spot;}
	ref_shader get_shader_point(){return s_point;}
	u32 get_transform_frame(){return m_transform_frame;}
	fmat4x4 get_transform(){return m_transform;}
	smapvis get_smapvis(){return svis;}
	virtual vis_data& get_homdata();

	virtual void set_type(LT type)
	{
		LightFlags.type = type;
	}
	virtual void get_sectors();
	virtual void set_active(bool b);
	virtual bool get_active()
	{
		return LightFlags.bActive;
	}
	virtual void set_shadow(bool b);
	virtual void set_position(const fvec3& P);
	virtual void set_rotation(const fvec3& D, const fvec3& R);
	virtual void set_cone(float angle);
	virtual void set_range(float R);
	virtual void set_virtual_size(float R){};
	virtual void set_color(const Fcolor& C)
	{
		color.set(C);
	}
	virtual void set_color(float r, float g, float b)
	{
		color.set(r, g, b, 1);
	}
	virtual void set_texture(LPCSTR name);

	virtual void spatial_move();
	virtual fvec3 spatial_sector_point();

	void set_frame_render(u32 frame)
	{
		frame_render = frame;
	}

	virtual IRender_Light* dcast_Light()
	{
		return this;
	}

	void transform_calc();
	bool camera_inside_volume() const;
	bool vis_prepare(u32 frame);
	void _export(light_Package& dest);

	float get_LOD();

	light();
	void TryToDeactivateLight();
	virtual ~light();
};
