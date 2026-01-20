#pragma once

// Микро-класс для хранения состояния камеры рендера
class ENGINE_API CRenderView
{
  public:
	// Векторы (бывшие vCamera...)
	Fvector Position;
	Fvector Direction;
	Fvector Top;
	Fvector Right;

	// Матрицы (бывшие m...)
	Fmatrix View;
	Fmatrix Project;
	Fmatrix ProjectHUD;		   // бывшая mProject_hud
	Fmatrix ViewProjection;	   // бывшая mFullTransform (View * Project)
	Fmatrix InvViewProjection; // бывшая mInvFullTransform

	// Предыдущий кадр (для интерполяции/velocity buffer)
	Fvector PositionSaved;
	Fmatrix ViewProjectionSaved;

	// Параметры
	float Fov;
	float Aspect;

  public:
	CRenderView();

	// Основные методы расчета
	void SetupView(const Fvector& pos, const Fvector& dir, const Fvector& top);
	void UpdateViewProjection(); // Расчет VP и InvVP
	void SaveState();			 // Сохранение текущего кадра как предыдущего
};
