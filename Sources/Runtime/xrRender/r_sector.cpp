#include "stdafx.h"
#include "r_sector.h"
#include "r_portal.h"
#include "render.h" // Для доступа к глобальному списку ресурсов при загрузке

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSector::CSector() : m_root_visual(nullptr)
{
}

CSector::~CSector()
{
	// Очищаем контейнеры.
	// Сами объекты (CPortal, IRender_Visual) удаляются в CRender::level_Unload,
	// так что здесь мы просто забываем указатели.
	m_portals.clear();
	m_root_visual = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Loading
//////////////////////////////////////////////////////////////////////

void CSector::Load(IReader& fs)
{
	// 1. Загрузка Порталов
	// В чанке fsP_Portals хранится список ID порталов (u16)
	if (fs.find_chunk(fsP_Portals))
	{
		u32 size = fs.find_chunk(fsP_Portals);
		R_ASSERT(0 == (size & 1)); // Размер должен быть кратен 2 (sizeof(u16))
		u32 count = size / 2;

		m_portals.reserve(count);

		while (count)
		{
			u16 portal_id = fs.r_u16();
			// Получаем указатель на портал из глобального пула рендера
			CPortal* portal = (CPortal*)RenderImplementation.getPortal(portal_id);
			m_portals.push_back(portal);
			count--;
		}
	}

	// 2. Загрузка Корневого Визуала (Геометрии)
	if (g_dedicated_server)
	{
		m_root_visual = nullptr;
	}
	else
	{
		if (fs.find_chunk(fsP_Root))
		{
			u32 size = fs.find_chunk(fsP_Root);
			R_ASSERT(size == 4); // ID визуала - это u32

			u32 visual_id = fs.r_u32();
			m_root_visual = RenderImplementation.getVisual(visual_id);
		}
	}
}
