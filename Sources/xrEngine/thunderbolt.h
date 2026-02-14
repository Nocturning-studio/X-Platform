// Rain.h: interface for the CRain class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ThunderboltH
#define ThunderboltH
#pragma once

// refs
class ENGINE_API IRender_DetailModel;
class ENGINE_API CLAItem;
class ENGINE_API CEnvironment;

struct SThunderboltDesc
{
	// geom
	IRender_DetailModel* l_model;
	// sound
	ref_sound snd;
	// gradient
	struct SFlare
	{
		float fOpacity;
		float2 fRadius;
		shared_str texture;
		shared_str shader;
		ref_shader hShader;
		SFlare()
		{
			fOpacity = 0;
			fRadius.set(0.f, 0.f);
		}
	};
	SFlare* m_GradientTop;
	SFlare* m_GradientCenter;
	shared_str name;
	CLAItem* color_anim;

  public:
	SThunderboltDesc();
	~SThunderboltDesc();
	void load(CInifile& pIni, shared_str const& sect);
	void create_top_gradient(CInifile& pIni, shared_str const& sect);
	void create_center_gradient(CInifile& pIni, shared_str const& sect);
};

struct SThunderboltCollection
{
	DEFINE_VECTOR(SThunderboltDesc*, DescVec, DescIt);
	DescVec palette;
	shared_str section;

  public:
	SThunderboltCollection();
	~SThunderboltCollection();
	void load(CInifile* pIni, CInifile* thunderbolts, LPCSTR sect);
	SThunderboltDesc* GetRandomDesc()
	{
		VERIFY(palette.size() > 0);
		return palette[Random.randI(palette.size())];
	}
};

#define THUNDERBOLT_CACHE_SIZE 8
//
class ENGINE_API CEffect_Thunderbolt
{
  protected:
	DEFINE_VECTOR(SThunderboltCollection*, CollectionVec, CollectionVecIt);
	CollectionVec collection;
	SThunderboltDesc* current;

  private:
	float4x4 current_transform;
	float3 current_direction;

	ref_geom hGeom_model;
	// states
	enum EState
	{
		stIdle,
		stWorking
	};
	EState state;

	ref_geom hGeom_gradient;

	float3 lightning_center;
	float lightning_size;
	float lightning_phase;

	float life_time;
	float current_time;
	float next_lightning_time;
	BOOL bEnabled;

  private:
	BOOL RayPick(const float3& s, const float3& d, float& range);
	void Bolt(shared_str id, float period, float life_time);

  public:
	CEffect_Thunderbolt();
	~CEffect_Thunderbolt();

	void OnFrame(shared_str id, float period, float duration);
	void Render();

	shared_str AppendDef(CEnvironment& environment, CInifile* pIni, CInifile* thunderbolts, LPCSTR sect);
};

#endif // ThunderboltH
