// LevelFogOfWar.h:  для карты уровня в одиночном режиме игры
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/UIWindow.h"
#include "alife_abstract_registry.h"

class CUICustomMap;
class CFogOfWarWrapper;

class CLevelFogOfWar : public IPureSerializeObject<IReader, IWriter>, public CUIWindow
{
  public:
	ref_shader hShader;
	ref_geom hGeom;

	shared_str m_level_name;
	Frect m_levelRect;
	u32 m_rowNum, m_colNum;
	xr_vector<bool> m_cells;

  public:
	CLevelFogOfWar();

	virtual void Draw();

	void Open(fvec2 pos);
	void Open(u32 row, u32 col, bool mask);
	void Init(const shared_str& level);
	void GetTexUVLT(fvec2& frame, u32 col, u32 row);
	ivec2 ConvertRealToLocal(const fvec2& src);
	Irect ConvertRealToLocal(const Frect& src);
	fvec2 ConvertLocalToReal(const ivec2& src);

	virtual void save(IWriter& stream);
	virtual void load(IReader& stream);
};

DEFINE_VECTOR(CLevelFogOfWar, FOG_STORAGE_T, FOG_STORAGE_IT);

typedef CALifeAbstractRegistry<u16, FOG_STORAGE_T> CFogOfWarRegistry;

class CFogOfWarMngr
{
	CFogOfWarWrapper* m_fogOfWarRegistry;
	FOG_STORAGE_T& GetFogStorage();

  public:
	CFogOfWarMngr();
	virtual ~CFogOfWarMngr();
	CLevelFogOfWar* GetFogOfWar(const shared_str& level_name);
};
