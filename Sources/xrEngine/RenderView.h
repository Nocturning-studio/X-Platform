#pragma once

// Микро-класс для хранения состояния камеры рендера
class ENGINE_API CRenderView
{
  public:
	// Векторы (бывшие vCamera...)
	float3 Position;
	float3 Direction;
	float3 Top;
	float3 Right;

	// Матрицы (бывшие m...)
	float4x4 View;
	float4x4 Project;
	float4x4 ProjectHUD;		   // бывшая mProject_hud
	float4x4 ViewProjection;	   // бывшая mFullTransform (View * Project)
	float4x4 InvViewProjection; // бывшая mInvFullTransform

	// Предыдущий кадр (для интерполяции/velocity buffer)
	float3 PositionSaved;
	float4x4 ViewProjectionSaved;

	// Параметры
	float Fov;
	float Aspect;

  public:
	CRenderView();

	// Основные методы расчета
	void SetupView(const float3& pos, const float3& dir, const float3& top);
	void UpdateViewProjection(); // Расчет VP и InvVP
	void SaveState();			 // Сохранение текущего кадра как предыдущего
};
