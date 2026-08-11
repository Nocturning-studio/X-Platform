#include "stdafx.h"
#include "uicursor.h"

#include "../xrEngine/CustomHUD.h"
#include "UI.h"
#include "HUDManager.h"
#include "ui/UIStatic.h"

#define C_DEFAULT D3DCOLOR_XRGB(0xff, 0xff, 0xff)

CUICursor::CUICursor()
{
	bVisible = false;
	vPos.set(0.f, 0.f);
	InitInternal();
	Engine.Events.Render.Add(this, 2);
}
//--------------------------------------------------------------------
CUICursor::~CUICursor()
{
	xr_delete(m_static);
	Engine.Events.Render.Remove(this);
}

void CUICursor::InitInternal()
{
	m_static = xr_new<CUIStatic>();
	m_static->InitTextureEx("ui\\ui_ani_cursor", "hud\\cursor");
	Frect rect;
	rect.set(0.0f, 0.0f, 40.0f, 40.0f);
	m_static->SetOriginalRect(rect);
	fvec2 sz;
	sz.set(rect.rb);
	if (UI()->is_16_9_mode())
		sz.x /= 1.2f;

	m_static->SetWndSize(sz);
	m_static->SetStretchTexture(true);
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;
void CUICursor::OnRender()
{
	//OPTICK_EVENT("CUICursor::OnRender");

	if (!IsVisible())
		return;
#ifdef DEBUG
	VERIFY(last_render_frame != Engine.TimeManager.GetFrameCount());
	last_render_frame = Engine.TimeManager.GetFrameCount();

	if (bDebug)
	{
		CGameFont* F = UI()->Font()->pFontDI;
		F->SetAligment(CGameFont::alCenter);
		F->SetHeightI(0.02f);
		F->OutSetI(0.f, -0.9f);
		F->SetColor(0xffffffff);
		fvec2 pt = GetCursorPosition();
		F->OutNext("%f-%f", pt.x, pt.y);
	}
#endif

	m_static->SetWndPos(vPos);
	m_static->Update();
	m_static->Draw();
}

fvec2 CUICursor::GetCursorPosition()
{
	return vPos;
}

fvec2 CUICursor::GetCursorPositionDelta()
{
	fvec2 res_delta;

	res_delta.x = vPos.x - vPrevPos.x;
	res_delta.y = vPos.y - vPrevPos.y;
	return res_delta;
}

void CUICursor::UpdateCursorPosition()
{

	POINT p;
	p.x = NULL;
	p.y = NULL;

	R_ASSERT(GetCursorPos(&p));
	R_ASSERT(ScreenToClient(Engine.WindowManager.GetHandle(), &p));

	vPrevPos = vPos;

	vPos.x = (float)p.x * (UI_BASE_WIDTH / (float)Device.dwWidth);
	vPos.y = (float)p.y * (UI_BASE_HEIGHT / (float)Device.dwHeight);

	clamp(vPos.x, 0.0f, UI_BASE_WIDTH);
	clamp(vPos.y, 0.0f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(fvec2 pos)
{
	clamp(pos.x, 0.f, UI_BASE_WIDTH);
	clamp(pos.y, 0.f, UI_BASE_HEIGHT);

	HWND hWnd = Engine.WindowManager.GetHandle();

	// 1) Текущая клиентка
	RECT rc{};
	GetClientRect(hWnd, &rc);
	const float cw = float(rc.right - rc.left);
	const float ch = float(rc.bottom - rc.top);

	// 2) UI -> клиентские пиксели
	POINT cpt;
	cpt.x = LONG(floorf(pos.x * (cw / UI_BASE_WIDTH) + 0.5f));
	cpt.y = LONG(floorf(pos.y * (ch / UI_BASE_HEIGHT) + 0.5f));

	// 4) Клиент -> экран и SetCursorPos
	ClientToScreen(hWnd, &cpt);
	SetCursorPos(cpt.x, cpt.y);

	// 5) Жёсткая синхронизация внутренних координат (не ждать физдвижения)
	if (0)//(m_b_use_win_cursor)
	{
		POINT pt{};
		GetCursorPos(&pt);		   // уже центр по экрану
		ScreenToClient(hWnd, &pt); // обратно в клиент
		const float sx = UI_BASE_WIDTH / cw;
		const float sy = UI_BASE_HEIGHT / ch;
		vPrevPos.set(pos);
		vPos.x = pt.x * sx;
		vPos.y = pt.y * sy;
	}
	else
	{
		vPrevPos.set(pos);
		vPos.set(pos);
	}
}
