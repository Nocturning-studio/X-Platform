#pragma once

#include "r_sector.h"
#include "r_portal.h"

// Scissor rectangle with depth info
struct ScissorRect : public Fbox2
{
	float depth;

	ScissorRect()
	{
		depth = 0.f;
		min.set(0, 0);
		max.set(1, 1);
	}
};

class CPortalTraverser
{
  public:
	enum ETraverseOptions
	{
		VQ_HOM = (1 << 0),	   // Использовать HOM (Occlusion Culling)
		VQ_SSA = (1 << 1),	   // Использовать Screen Space Area culling
		VQ_SCISSOR = (1 << 2), // Использовать Scissor test
		VQ_FADE = (1 << 3),	   // Рисовать затухание порталов
	};

	// Результат видимости для одного сектора
	struct SectorVisibility
	{
		CSector* sector;
		xr_vector<CFrustum> frustums;
		xr_vector<ScissorRect> scissors;
		ScissorRect merged_scissor; // Объединенный сциссор для оптимизации

		SectorVisibility() : sector(nullptr)
		{
			merged_scissor.invalidate();
		}
	};

  private:
	// === Контекст текущего обхода ===
	u32 m_options;
	fvec3 m_view_pos;
	fmat4x4 m_xform;
	fmat4x4 m_xform_proj; // View * Proj * ViewPort
	CSector* m_start_sector;

	// === Результаты и Кэш (State) ===
	// Список видимых секторов с их фрустумами
	xr_vector<SectorVisibility> m_visible_sectors;

	// Список посещенных порталов (чтобы не ходить кругами)
	// Используем std::vector и linear search, т.к. N < 100 это быстрее hash_set
	xr_vector<CPortal*> m_visited_portals;

	// Данные для отрисовки "тумана" в порталах (Fade)
	xr_vector<std::pair<CPortal*, float>> m_fade_portals;

	// Ресурсы рендера
	ref_shader m_shader_fade;
	ref_geom m_geom_fade;

  public:
	CPortalTraverser();

	// Управление ресурсами
	void CreateResources();
	void DestroyResources();

	// Сброс состояния перед кадром
	void Reset();

	// Основной метод запуска обхода
	void Traverse(CSector* start, CFrustum& frustum, fvec3& view_pos, fmat4x4& xform, u32 options);

	// Доступ к результатам
	const xr_vector<SectorVisibility>& GetVisibleSectors() const
	{
		return m_visible_sectors;
	}

	// Рендер затухания (вызывается в конце кадра)
	void RenderFade();

  private:
	// Рекурсивное ядро обхода
	void RecursiveTraverse(CSector* current_sector, const CFrustum& cur_frustum, const ScissorRect& cur_scissor);

	// Хелпер добавления сектора в результаты
	SectorVisibility& GetOrAddSectorData(CSector* sector);
};
