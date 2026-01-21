#include "stdafx.h"
#include "RenderView.h"
#include "R_Backend.h" // Для RenderBackend
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

void CRenderView::SetupView(const Fvector& pos, const Fvector& dir, const Fvector& top)
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
	RenderBackend.set_transform_view(View);
	RenderBackend.set_transform_project(Project);
}

void CRenderView::SaveState()
{
	PositionSaved = Position;
	ViewProjectionSaved = ViewProjection;
}
