#pragma once

#include "ui_base.h"
#include "UIStaticItem.h"

class CUIStatic;

class CUICursor : public pureRender
{
	bool bVisible;
	float2 vPos;
	float2 vPrevPos;

	CUIStatic* m_static;
	void InitInternal();

  public:
	CUICursor();
	virtual ~CUICursor();
	virtual void OnRender();

	float2 GetCursorPositionDelta();

	float2 GetCursorPosition();
	void SetUICursorPosition(float2 pos);
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
