#include "stdafx.h"
#include "UIHint.h"
#include "UIStatic.h"
#include <ui/UIBtnHint.h>

CUIHint::CUIHint()
{
	Device.seqRender.Add(this, 0);

	m_ownerWnd = nullptr;
	m_hint = nullptr;
}

CUIHint::~CUIHint()
{
	Device.seqRender.Remove(this);
}

extern CUIButtonHint* g_btnHint;

void CUIHint::OnRender()
{
	//OPTICK_EVENT("CUIHint::OnRender");

	bool bGlobalHierarchyVisible = false;

	if (m_ownerWnd)
	{
		// Проверяем всю цепочку окон от владельца до корня
		bGlobalHierarchyVisible = true;

		CUIWindow* pCurrent = m_ownerWnd;
		while (pCurrent)
		{
			if (!pCurrent->IsShown())
			{
				bGlobalHierarchyVisible = false;
				break;
			}
			pCurrent = pCurrent->GetParent();
		}
	}

	// Обработка кастомного хинта
	if (m_hint)
	{
		if (bGlobalHierarchyVisible)
		{
			m_hint->Update();
			Draw(); // Рисуем только если вся цепочка видима
		}
		else
		{
			// Принудительно скрываем, если родитель скрыт
			m_hint->SetVisible(false);
		}
	}

	// Обработка глобального хинта (g_btnHint)
	// Используем этот метод как "сторожа" для g_btnHint
	if (m_ownerWnd && g_btnHint->Owner() == m_ownerWnd)
	{
		if (!bGlobalHierarchyVisible)
		{
			g_btnHint->Discard();
		}
	}
}
