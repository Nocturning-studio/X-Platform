#pragma once
#include "XR_IOConsole.h"
#include "IGame_Level.h"
#include <windows.h>
#include <process.h> // для _beginthreadex

class ENGINE_API CTextConsole : public CConsole
{
  private:
	typedef CConsole inherited;

  private:
	HANDLE m_hConsoleThread;
	CRITICAL_SECTION m_csCmdQueue;

	// Очередь команд от потока к движку
	xr_vector<shared_str> m_cmd_queue;

	HANDLE m_hStdOut;
	u32 m_dwLastLogIndex;

	volatile bool m_bConsoleRunning;

	void ProcessOutput();
	WORD GetColorByTag(char tag);

	// Статическая функция для потока
	static unsigned __stdcall ConsoleThreadEntry(void* pArgs);
	void ThreadLoop();

  public:
	CTextConsole();
	virtual ~CTextConsole();

	virtual void Initialize();
	virtual void Destroy();

	virtual void OnRender();
	virtual void OnFrame();
	virtual void OnPaint();

	void AddString(LPCSTR string);

}; // class TextConsole
