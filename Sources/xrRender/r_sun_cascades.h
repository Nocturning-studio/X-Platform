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

namespace sun
{

struct ray
{
	ray( ) { }
	ray( Fvector3 const& _P, Fvector3 const& _D ):	P(_P), D(_D) { }

	Fvector3 D;
	Fvector3 P;
};

struct cascade 
{
	cascade () : reset_chain( false )	{}

	Fmatrix			transform;
	xr_vector<ray>	rays;
	float			size;
	float			bias;
	bool			reset_chain;
};

} //namespace sun