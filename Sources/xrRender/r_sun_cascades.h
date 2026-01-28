#pragma once

// =========================================================================
//  Вспомогательные структуры и функции
// =========================================================================

// Структура для передачи данных между Gather и Draw
struct ShadowCascadeWorkItem
{
	SceneGraphPacket packet; // Локальный пакет для сбора

	// Данные матриц и отсечения
	Fmatrix cull_transform;
	Fvector3 cull_COP;
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

namespace Sun
{

struct Ray
{
	Fvector3 Direction;
	Fvector3 Position;

	Ray()
	{
	}
	Ray(Fvector3 const& _P, Fvector3 const& _D) : Position(_P), Direction(_D)
	{
	}
};

struct Cascade 
{
	Cascade () : reset_chain( false )	{}

	Fmatrix			transform;
	xr_vector<Ray>	rays;
	float			size;
	float			bias;
	bool			reset_chain;
};

} //namespace sun