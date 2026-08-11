#include "stdafx.h"
#include "resource.h"
#include "LogoWindow.h"

static BOOL CALLBACK logoDlgProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		break;
	case WM_COMMAND:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LogoWindow::LogoWindow()
{
	m_logo = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, logoDlgProc);
}

LogoWindow::~LogoWindow()
{
	DestroyWindow(m_logo);
	m_logo = NULL;
}

void LogoWindow::Show()
{
	SetWindowPos(m_logo, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	UpdateWindow(m_logo);
}

void LogoWindow::Hide()
{
	SetWindowPos(m_logo, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
	UpdateWindow(m_logo);
}
