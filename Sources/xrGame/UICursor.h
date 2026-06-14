#pragma once

#include "ui_base.h"
#include "UIStaticItem.h"

class CUIStatic;

class CUICursor : public pureRender
{
	bool bVisible;
	fvec2 vPos;
	fvec2 vPrevPos;

	CUIStatic* m_static;
	void InitInternal();

  public:
	CUICursor();
	virtual ~CUICursor();
	virtual void OnRender();

	fvec2 GetCursorPositionDelta();

	fvec2 GetCursorPosition();
	void SetUICursorPosition(fvec2 pos);
	void UpdateCursorPosition();

	bool IsVisible()
	{
		return bVisible;
	}
	void Show()
	{
		bVisible = true;
	}
	void Hide()
	{
		bVisible = false;
	}
	CUIStatic* GetStatic()
	{
		return m_static;
	}
};
