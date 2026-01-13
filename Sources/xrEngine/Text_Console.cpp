#include "stdafx.h"
#include "Text_Console.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>

extern char const* const ioc_prompt;

// Цвета
#define C_DEFAULT (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define C_WHITE (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)
#define C_RED (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define C_GREEN (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define C_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define C_BLUE (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define C_GREY (FOREGROUND_INTENSITY)

CTextConsole::CTextConsole()
{
	m_hConsoleThread = NULL;
	m_hStdOut = INVALID_HANDLE_VALUE;
	m_dwLastLogIndex = 0;
	m_bConsoleRunning = false;

	InitializeCriticalSection(&m_csCmdQueue);
}

CTextConsole::~CTextConsole()
{
	DeleteCriticalSection(&m_csCmdQueue);
}

void CTextConsole::Initialize()
{
	inherited::Initialize();

	// Привязка глобальной переменной
	if (!Console)
		Console = this;

	bool bHasConsole = (GetConsoleWindow() != NULL);
	if (!bHasConsole && AllocConsole())
		bHasConsole = true;

	if (bHasConsole)
	{
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		m_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTitle("STALKER Dedicated Server Console");

		COORD bufferSize = {120, 3000};
		SetConsoleScreenBufferSize(m_hStdOut, bufferSize);

		// Цветной приветственный текст
		SetConsoleTextAttribute(m_hStdOut, C_GREEN);
		printf("================================================================================\n");
		printf(" STALKER DEDICATED SERVER CONSOLE READY\n");
		printf("================================================================================\n");
		SetConsoleTextAttribute(m_hStdOut, C_DEFAULT);

		// Рисуем промпт сразу
		printf("%s", ioc_prompt);
	}

	if (LogFile)
		m_dwLastLogIndex = LogFile->size();

	// --- ЗАПУСК ПОТОКА ВВОДА ---
	m_bConsoleRunning = true;
	unsigned threadID;
	m_hConsoleThread = (HANDLE)_beginthreadex(NULL, 0, ConsoleThreadEntry, this, 0, &threadID);

	// !!! САМОЕ ВАЖНОЕ ИСПРАВЛЕНИЕ !!!
	// Принудительно регистрируем консоль в цикле обновлений движка.
	// 2 = приоритет (чтобы срабатывало после ввода, но до рендера, хотя на сервере это не критично)
	Device.seqFrame.Add(this, REG_PRIORITY_NORMAL);
}

void CTextConsole::Destroy()
{
	// !!! УДАЛЯЕМ ИЗ ЦИКЛА !!!
	Device.seqFrame.Remove(this);

	inherited::Destroy();

	m_bConsoleRunning = false;
	if (m_hConsoleThread)
	{
		TerminateThread(m_hConsoleThread, 0);
		CloseHandle(m_hConsoleThread);
	}
	// FreeConsole(); // Можно оставить или убрать по вкусу
}

void CTextConsole::OnRender()
{
}
void CTextConsole::OnPaint()
{
}
void CTextConsole::AddString(LPCSTR string)
{
}

WORD CTextConsole::GetColorByTag(char tag)
{
	switch (tag)
	{
	case '!':
		return C_RED;
	case '~':
		return C_YELLOW;
	case '*':
		return C_GREY;
	case '-':
		return C_GREEN;
	case '#':
		return C_BLUE;
	default:
		return C_WHITE;
	}
}

// -----------------------------------------------------------
// ПОТОК ВВОДА
// -----------------------------------------------------------
unsigned __stdcall CTextConsole::ConsoleThreadEntry(void* pArgs)
{
	CTextConsole* pThis = (CTextConsole*)pArgs;
	pThis->ThreadLoop();
	return 0;
}

void CTextConsole::ThreadLoop()
{
	char buffer[1024];

	while (m_bConsoleRunning)
	{
		if (fgets(buffer, 1024, stdin))
		{
			// Чистим любые спецсимволы в конце строки
			size_t len = strlen(buffer);
			while (len > 0 && (unsigned char)buffer[len - 1] <= 32)
			{
				buffer[len - 1] = 0;
				len--;
			}

			if (len > 0)
			{
				EnterCriticalSection(&m_csCmdQueue);
				m_cmd_queue.push_back(buffer);
				LeaveCriticalSection(&m_csCmdQueue);
			}
		}
		Sleep(1);
	}
}

// -----------------------------------------------------------
// ГЛАВНЫЙ ПОТОК (ENGINE LOOP)
// -----------------------------------------------------------
void CTextConsole::ProcessOutput()
{
	if (!LogFile)
		return;

	u32 curSize = LogFile->size();
	bool hasNewData = curSize > m_dwLastLogIndex;

	if (hasNewData)
	{
		// Если пришли новые логи - нужно "перебить" текущую строку ввода
		// Стандартный способ в консолях:
		// 1. \r (в начало)
		// 2. Стереть пробелами
		// 3. Вывести лог
		// 4. Напечатать промпт заново

		DWORD written;
		WriteConsole(m_hStdOut, "\r", 1, &written, NULL);
		// Просто перезатираем текущую строку вывода логами

		for (u32 i = m_dwLastLogIndex; i < curSize; ++i)
		{
			const shared_str& msg = (*LogFile)[i];
			if (!msg.size())
				continue;

			LPCSTR str = msg.c_str();
			WORD color = GetColorByTag(str[0]);
			if (str[0] == ' ' && msg.size() > 1)
				color = GetColorByTag(str[1]);

			SetConsoleTextAttribute(m_hStdOut, color);
			// Важно использовать printf/WriteConsole, синхронно с тем, что использует наш поток
			WriteConsole(m_hStdOut, str, (DWORD)xr_strlen(str), &written, NULL);
			WriteConsole(m_hStdOut, "\n", 1, &written, NULL);
		}

		SetConsoleTextAttribute(m_hStdOut, C_DEFAULT);
		m_dwLastLogIndex = curSize;

		// Восстанавливаем приглашение к вводу
		WriteConsole(m_hStdOut, ioc_prompt, (DWORD)xr_strlen(ioc_prompt), &written, NULL);

		// Нюанс: мы не можем восстановить то, что пользователь уже успел напечатать наполовину (до Enter),
		// так как буфер лежит внутри fgets. Это компромисс системной консоли.
		// Зато ввод гарантированно работает.
	}
}

void CTextConsole::OnFrame()
{
	// inherited::OnFrame(); // НЕ вызываем родительский метод, там логика GUI нам не нужна

	// 1. Выводим новые логи
	ProcessOutput();

	// 2. Обрабатываем очередь команд
	// Используем обычный EnterCriticalSection, так как он очень быстрый (мьютекс в user-space)
	EnterCriticalSection(&m_csCmdQueue);

	if (!m_cmd_queue.empty())
	{
		// Быстро копируем очередь себе
		xr_vector<shared_str> todo = m_cmd_queue;
		m_cmd_queue.clear();
		LeaveCriticalSection(&m_csCmdQueue);

		// Выполняем команды
		for (u32 i = 0; i < todo.size(); ++i)
		{
			LPCSTR cmd_str = todo[i].c_str();

			// Логируем попытку выполнения, чтобы видеть, что движок принял команду
			// Msg — это функция движка, она попадет в лог и на экран сама через ProcessOutput
			Msg("sv_console_execute: %s", cmd_str);

			// Выполняем команду
			this->Execute(cmd_str);
		}

		// Рисуем промпт после выполнения всех команд
		DWORD written;
		WriteConsole(m_hStdOut, ioc_prompt, (DWORD)xr_strlen(ioc_prompt), &written, NULL);
	}
	else
	{
		LeaveCriticalSection(&m_csCmdQueue);
	}
}
