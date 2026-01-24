#pragma once

// Forward declarations
class CSector;

class CPortal : public IRender_Portal
{
  public:
	// Геометрические данные
	svector<Fvector, 8> m_vertices;
	Fplane m_plane;
	Fsphere m_sphere;

	// Связи (Граф)
	CSector* m_front_sector;
	CSector* m_back_sector;

  public:
	CPortal();
	virtual ~CPortal();

	// Инициализация (вызывается при загрузке)
	void Setup(Fvector* v_ptr, int v_count, CSector* face, CSector* back);

	// Доступ к данным
	IC const svector<Fvector, 8>& GetVertices() const
	{
		return m_vertices;
	}
	IC const Fsphere& GetSphere() const
	{
		return m_sphere;
	}
	IC const Fplane& GetPlane() const
	{
		return m_plane;
	}

	// Логика графа (Read-Only)
	IC CSector* GetFrontSector() const
	{
		return m_front_sector;
	}
	IC CSector* GetBackSector() const
	{
		return m_back_sector;
	}

	// Возвращает сектор, противоположный переданному
	IC CSector* GetOppositeSector(CSector* current_sector) const
	{
		return (current_sector == m_front_sector) ? m_back_sector : m_front_sector;
	}

	// Определяет, в какой сектор смотрит точка
	CSector* GetSectorFacing(const Fvector& v) const
	{
		return (m_plane.classify(v) > 0) ? m_front_sector : m_back_sector;
	}

	// Определяет, какой сектор находится "сзади" точки (откуда смотрим)
	CSector* GetSectorBack(const Fvector& v) const
	{
		return (m_plane.classify(v) > 0) ? m_back_sector : m_front_sector;
	}

#ifdef DEBUG
	virtual void OnRender(); // Debug draw
#endif
};