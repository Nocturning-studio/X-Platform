#pragma once

class LogoWindow
{
  private:
	HWND m_logo;

  public:
	LogoWindow();
	~LogoWindow();

	void Show();
	void Hide();
};
