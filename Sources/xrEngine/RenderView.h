#pragma once

// Микро-класс для хранения состояния камеры рендера
class ENGINE_API CRenderView
{
  public:
	// Векторы (бывшие vCamera...)
	fvec3 Position;
	fvec3 Direction;
	fvec3 Top;
	fvec3 Right;

	// Матрицы (бывшие m...)
	fmat4x4 View;
	fmat4x4 Project;
	fmat4x4 ProjectHUD;		   // бывшая mProject_hud
	fmat4x4 ViewProjection;	   // бывшая mFullTransform (View * Project)
	fmat4x4 InvViewProjection; // бывшая mInvFullTransform

	// Предыдущий кадр (для интерполяции/velocity buffer)
	fvec3 PositionSaved;
	fmat4x4 ViewProjectionSaved;

	// Параметры
	float Fov;
	float Aspect;

  public:
	CRenderView();

	// Основные методы расчета
	void SetupView(const fvec3& pos, const fvec3& dir, const fvec3& top);
	void UpdateViewProjection(); // Расчет VP и InvVP
	void SaveState();			 // Сохранение текущего кадра как предыдущего
};
