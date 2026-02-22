#include "stdafx.h"
#include "RenderView.h"
#include "R_Backend.h" // Для RenderBackendLegacy
#include <d3dx9.h>

CRenderView::CRenderView()
{
	Position.set(0, 0, 0);
	Direction.set(0, 0, 1);
	Top.set(0, 1, 0);
	Right.set(1, 0, 0);

	View.identity();
	Project.identity();
	ProjectHUD.identity();
	ViewProjection.identity();
	InvViewProjection.identity();

	PositionSaved.set(0, 0, 0);
	ViewProjectionSaved.identity();

	Fov = 90.f;
	Aspect = 1.f;
}

void CRenderView::SetupView(const float3& pos, const float3& dir, const float3& top)
{
	Position.set(pos);
	Direction.set(dir);
	Top.set(top);

	// Рассчитываем вектор Right (право)
	Right.crossproduct(Top, Direction);

	// Строим матрицу вида
	View.build_camera_dir(Position, Direction, Top);
}

void CRenderView::UpdateViewProjection()
{
	// VP = P * V
	ViewProjection.mul(Project, View);

	// Обратная VP (используем D3DX для точности, как в оригинале)
	D3DXMatrixInverse((D3DXMATRIX*)&InvViewProjection, 0, (D3DXMATRIX*)&ViewProjection);

	// Сразу отправляем в бекенд (так как это данные рендера)
	RenderBackendLegacy.set_transform_view(View);
	RenderBackendLegacy.set_transform_project(Project);
}

void CRenderView::SaveState()
{
	PositionSaved = Position;
	ViewProjectionSaved = ViewProjection;
}
