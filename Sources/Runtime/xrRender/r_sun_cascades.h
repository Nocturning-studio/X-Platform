#pragma once

// =========================================================================
//  Вспомогательные структуры и функции
// =========================================================================

// Структура для передачи данных между Gather и Draw
struct ShadowCascadeWorkItem
{
	SceneGraphPacket packet; // Локальный пакет для сбора

	// Данные матриц и отсечения
	fmat4x4 cull_transform;
	fvec3 cull_COP;
	CFrustum cull_frustum;
	CSector* cull_sector;

	// Конструктор: Инициализируем ресурсы, так как мы создаемся внутри кадра (Device жив)
	ShadowCascadeWorkItem()
	{
	}

	~ShadowCascadeWorkItem()
	{
	}
};

struct SunCascadeBuffer
{
	ShadowCascadeWorkItem* items[3];

	SunCascadeBuffer()
	{
		for (int i = 0; i < 3; ++i)
			items[i] = nullptr;
	}

	void Init()
	{
		for (int i = 0; i < 3; ++i)
		{
			if (!items[i])
				items[i] = xr_new<ShadowCascadeWorkItem>();
			items[i]->packet.InitResources();
		}
	}

	void Destroy()
	{
		for (int i = 0; i < 3; ++i)
		{
			if (items[i])
			{
				items[i]->packet.FreeResources();
				xr_delete(items[i]);
			}
		}
	}

	void Clear()
	{
		for (int i = 0; i < 3; ++i)
		{
			if (items[i])
				items[i]->packet.Clear();
		}
	}
};


namespace Sun
{

struct Ray
{
	fvec3 Direction;
	fvec3 Position;

	Ray()
	{
	}
	Ray(fvec3 const& _P, fvec3 const& _D) : Position(_P), Direction(_D)
	{
	}
};

struct Cascade 
{
	Cascade () : reset_chain( false )	{}

	xr_vector<Ray>	rays;
	float			size;
	bool			reset_chain;
};

} //namespace sun