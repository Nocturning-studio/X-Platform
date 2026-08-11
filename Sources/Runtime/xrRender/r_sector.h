#pragma once

#include "r_portal.h"
#include "..\xrEngine\xrLevel.h" // Для fsP_Portals, fsP_Root

class CSector : public IRender_Sector
{
  private:
	IRender_Visual* m_root_visual; // Корневая геометрия сектора (Static Geometry)
	xr_vector<CPortal*> m_portals; // Список порталов, ведущих из этого сектора

  public:
	CSector();
	virtual ~CSector();

	// Загрузка данных из .level файла
	void Load(IReader& fs);

	// === Accessors (Thread-Safe getters) ===

	IC IRender_Visual* GetRootVisual() const
	{
		return m_root_visual;
	}

	IC const xr_vector<CPortal*>& GetPortals() const
	{
		return m_portals;
	}

	// === IRender_Sector Interface Implementation ===

	virtual IRender_Visual* root()
	{
		return GetRootVisual();
	}
};
