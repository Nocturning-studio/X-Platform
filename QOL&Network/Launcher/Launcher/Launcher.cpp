// Launcher.cpp - Game Launcher with Update System and Feedback
// Компилировать как Windows Application
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <future>
#include <atomic>
#include <map>
#include <codecvt>
#include <commctrl.h>
#include <windowsx.h>
#include <sstream>
#include "TinyXML\tinyxml2.h"

using namespace tinyxml2;

#pragma comment(lib, "comctl32.lib")
#pragma comment(                                                                                                       \
	linker,                                                                                                            \
	"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ============================================
// КОНСТАНТЫ И НАСТРОЙКИ
// ============================================

const std::wstring GAME_EXE_NAME = L"xrEngine.exe";
const std::wstring FS_LTX_NAME = L"game_filesystem.ltx";
const std::wstring LOG_FILE_NAME = L"xray_engine.log";
const std::wstring BIN_DIR_REL = L"binaries";
const std::wstring LOG_SENDER_REL = L"network\\MailService.exe";
const std::wstring UPDATE_SERVICE_REL = L"binaries\\network\\UpdateService.exe";
const std::wstring BG_IMAGE_NAME = L"launcher_bg.bmp";
const std::wstring BENCHMARK_ARGS = L"-benchmark demo_escape";

// ID элементов управления
#define ID_BTN_PLAY 101
#define ID_BTN_BENCH 102
#define ID_BTN_LOG 103
#define ID_BTN_FEEDBACK 104
#define ID_BTN_SEND_FB 105
#define ID_EDIT_FB 106
#define ID_BTN_CLOSE 107
#define ID_BTN_SEND_WITH_WARNING 108
#define ID_BTN_WARNING_YES 109
#define ID_BTN_WARNING_NO 110
#define ID_BTN_LOG_YES 113
#define ID_BTN_LOG_NO 114
#define ID_BTN_UPDATE 120

// Пользовательские сообщения
#define WM_FEEDBACK_SENT (WM_USER + 200)
#define WM_UPDATE_PROGRESS (WM_USER + 201)

// ============================================
// СТРУКТУРЫ ДАННЫХ
// ============================================

struct EmailSubjects
{
	std::wstring logSubject = L"SHOC:ReLighted-Crash-Report";
	std::wstring feedbackSubject = L"SHOC:ReLighted-User-Feedback";
	std::wstring benchmarkSubject = L"SHOC:ReLighted-Benchmark-Results";
	std::wstring warningSubject = L"SHOC:ReLighted-Error-Report";
};

enum UIControlType
{
	UI_BUTTON,
	UI_EDIT,
	UI_LABEL,
	UI_CHECKBOX
};

struct UIControl
{
	std::string name;
	UIControlType type;
	int id;
	int x, y, w, h;
	std::wstring text;
	std::wstring tooltip;
	bool visible;
	bool enabled;
};

struct UIWindow
{
	std::string name;
	std::wstring title;
	int width, height;
	std::vector<UIControl> controls;
};

struct SkinConfig
{
	// Главное окно
	int winW = 700;
	int winH = 400;
	std::wstring bgImage = L"launcher_bg.bmp";
	std::wstring title = L"Game Launcher";
	std::vector<UIControl> mainWindowControls;

	// Окна с значениями по умолчанию
	UIWindow feedbackWindow = {"FeedbackWindow", L"Обратная связь", 500, 400, {}};
	UIWindow feedbackWarningWindow = {"FeedbackWarningWindow", L"Предупреждение", 400, 200, {}};
	UIWindow logWarningWindow = {"LogWarningWindow", L"Отправка лога", 400, 200, {}};
};

// ============================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================

// Конфигурация и темы
SkinConfig g_Skin;
EmailSubjects g_EmailSubjects;
HBITMAP g_hBgImage = NULL;

// Состояние приложения
std::atomic<bool> g_IsSendingFeedback = false;
HWND g_hCurrentFeedbackWnd = nullptr;
bool g_Dragging = false;
POINT g_DragStart;

// Пути
std::wstring g_LauncherDir;
std::wstring g_LauncherResourcesDir;
std::wstring g_BinDir;
std::wstring g_GamePath;
std::wstring g_SenderPath;
std::wstring g_UpdateServicePath;

// ============================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================

// Основные оконные процедуры
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FeedbackWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FeedbackWarningWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LogWarningWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Вспомогательные функции
void InitializePaths();
void LoadSkinConfig(const std::wstring& xmlPath);
void LogMessage(const std::wstring& message);
void RunGame(const std::wstring& args = L"");
void SendLogFile();
void OpenFeedbackWarning(HWND hParent);
void OpenFeedbackForm(HWND hParent);
void OpenLogWarningWindow(HWND hParent);
void CheckForUpdates(HWND hWnd, bool silent = false);
bool CheckUpdateDependencies();

// Функции работы с файлами и процессами
std::wstring GetExeDir();
std::wstring JoinPaths(const std::wstring& part1, const std::wstring& part2);
bool FileExists(const std::wstring& filepath);
int ExecuteProcess(const std::wstring& cmdLine, const std::wstring& workDir, bool showWindow = false);
void SendFeedbackAsync(const std::wstring& filePath, const std::wstring& subject, HWND hWnd);

// Утилиты
std::wstring ConvertUtf8ToWide(const char* str);
std::wstring ConvertAnsiToWide(const std::string& str);
std::string ConvertWideToUtf8(const std::wstring& wstr);
UIControlType ConvertStringToControlType(const std::string& typeStr);
HWND CreateCenteredWindow(HWND hParent, const std::wstring& className, const std::wstring& title, int width, int height,
						  WNDPROC wndProc, bool registerClass = false);

// ============================================
// РЕАЛИЗАЦИЯ ФУНКЦИЙ
// ============================================

// --- ЛОГИРОВАНИЕ ---
void LogMessage(const std::wstring& message)
{
	std::wofstream logFile(L"launcher_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		logFile << L"[" << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"] " << message << std::endl;
		logFile.close();
	}
}

// --- РАБОТА С ПУТЯМИ ---
std::wstring GetExeDir()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::wstring path(buffer);
	return path.substr(0, path.find_last_of(L"\\/"));
}

std::wstring JoinPaths(const std::wstring& part1, const std::wstring& part2)
{
	if (part1.empty())
		return part2;
	if (part2.empty())
		return part1;

	std::wstring result = part1;
	if (result.back() != L'\\' && result.back() != L'/')
		result += L'\\';

	// Убираем начальные разделители из второй части
	std::wstring part2Clean = part2;
	while (!part2Clean.empty() && (part2Clean[0] == L'\\' || part2Clean[0] == L'/'))
		part2Clean.erase(0, 1);

	return result + part2Clean;
}

bool FileExists(const std::wstring& filepath)
{
	DWORD fileAttr = GetFileAttributesW(filepath.c_str());
	return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

// --- КОНВЕРТАЦИЯ СТРОК ---
std::wstring ConvertUtf8ToWide(const char* str)
{
	if (!str || strlen(str) == 0)
		return L"";

	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (sizeNeeded <= 0)
		return L"";

	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], sizeNeeded);
	result.resize(sizeNeeded - 1);
	return result;
}

std::wstring ConvertAnsiToWide(const std::string& str)
{
	if (str.empty())
		return L"";

	int size = MultiByteToWideChar(1251, 0, str.c_str(), -1, NULL, 0);
	if (size <= 0)
		return L"";

	std::vector<wchar_t> buffer(size);
	MultiByteToWideChar(1251, 0, str.c_str(), -1, &buffer[0], size);
	return std::wstring(buffer.data());
}

std::string ConvertWideToUtf8(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";

	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	if (size <= 0)
		return "";

	std::vector<char> buffer(size);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], size, NULL, NULL);
	return std::string(buffer.data());
}

// --- РАБОТА С ПРОЦЕССАМИ ---
int ExecuteProcess(const std::wstring& cmdLine, const std::wstring& workDir, bool showWindow)
{
	LogMessage(L"ExecuteProcess: " + cmdLine);

	STARTUPINFOW si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);

	if (!showWindow)
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
	cmdBuffer.push_back(0);

	std::vector<wchar_t> workDirBuffer;
	const wchar_t* workDirPtr = nullptr;

	if (!workDir.empty())
	{
		workDirBuffer = std::vector<wchar_t>(workDir.begin(), workDir.end());
		workDirBuffer.push_back(0);
		workDirPtr = &workDirBuffer[0];
	}

	if (!CreateProcessW(nullptr, &cmdBuffer[0], nullptr, nullptr, FALSE, 0, nullptr, workDirPtr, &si, &pi))
	{
		DWORD err = GetLastError();
		LogMessage(L"CreateProcess failed: " + std::to_wstring(err));
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode = 0;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return static_cast<int>(exitCode);
}

// --- ЗАГРУЗКА КОНФИГУРАЦИИ ---
UIControlType ConvertStringToControlType(const std::string& typeStr)
{
	if (typeStr == "button")
		return UI_BUTTON;
	if (typeStr == "edit")
		return UI_EDIT;
	if (typeStr == "label")
		return UI_LABEL;
	if (typeStr == "checkbox")
		return UI_CHECKBOX;
	return UI_BUTTON;
}

void LoadSkinConfig(const std::wstring& xmlPath)
{
	tinyxml2::XMLDocument doc;

	// Открываем файл
	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, xmlPath.c_str(), L"rb");

	if (err != 0 || fp == nullptr)
	{
		MessageBoxW(nullptr, (L"Не удалось открыть XML файл:\n" + xmlPath).c_str(), L"Ошибка", MB_ICONERROR);
		return;
	}

	// Загружаем XML
	XMLError xmlErr = doc.LoadFile(fp);
	fclose(fp);

	if (xmlErr != XML_SUCCESS)
	{
		MessageBoxW(nullptr, L"Ошибка парсинга XML файла", L"Ошибка", MB_ICONERROR);
		return;
	}

	// Получаем корневой элемент
	XMLElement* root = doc.FirstChildElement("Skin");
	if (!root)
	{
		MessageBoxW(nullptr, L"Корневой элемент <Skin> не найден", L"Ошибка", MB_ICONERROR);
		return;
	}

	// Загружаем параметры главного окна
	XMLElement* winEl = root->FirstChildElement("Window");
	if (winEl)
	{
		winEl->QueryIntAttribute("width", &g_Skin.winW);
		winEl->QueryIntAttribute("height", &g_Skin.winH);

		const char* bg = winEl->Attribute("background");
		if (bg)
			g_Skin.bgImage = ConvertUtf8ToWide(bg);

		const char* tit = winEl->Attribute("title");
		if (tit)
			g_Skin.title = ConvertUtf8ToWide(tit);
	}

	// Загружаем элементы управления главного окна
	g_Skin.mainWindowControls.clear();
	XMLElement* controlEl = root->FirstChildElement("Control");
	while (controlEl)
	{
		UIControl control;

		const char* name = controlEl->Attribute("name");
		if (name)
			control.name = name;

		const char* type = controlEl->Attribute("type");
		if (type)
			control.type = ConvertStringToControlType(type);

		controlEl->QueryIntAttribute("id", &control.id);
		controlEl->QueryIntAttribute("x", &control.x);
		controlEl->QueryIntAttribute("y", &control.y);
		controlEl->QueryIntAttribute("width", &control.w);
		controlEl->QueryIntAttribute("height", &control.h);

		const char* text = controlEl->Attribute("text");
		if (text)
			control.text = ConvertUtf8ToWide(text);

		const char* tooltip = controlEl->Attribute("tooltip");
		if (tooltip)
			control.tooltip = ConvertUtf8ToWide(tooltip);

		control.visible = true;
		const char* visible = controlEl->Attribute("visible");
		if (visible && std::string(visible) == "false")
			control.visible = false;

		control.enabled = true;
		const char* enabled = controlEl->Attribute("enabled");
		if (enabled && std::string(enabled) == "false")
			control.enabled = false;

		// Сопоставляем имена с ID
		if (control.name == "play_btn")
			control.id = ID_BTN_PLAY;
		else if (control.name == "bench_btn")
			control.id = ID_BTN_BENCH;
		else if (control.name == "log_btn")
			control.id = ID_BTN_LOG;
		else if (control.name == "feedback_btn")
			control.id = ID_BTN_FEEDBACK;
		else if (control.name == "close_btn")
			control.id = ID_BTN_CLOSE;
		else if (control.name == "update_btn")
			control.id = ID_BTN_UPDATE;

		if (!control.name.empty())
			g_Skin.mainWindowControls.push_back(control);

		controlEl = controlEl->NextSiblingElement("Control");
	}

	// Загружаем окно обратной связи
	XMLElement* feedbackEl = root->FirstChildElement("FeedbackWindow");
	if (feedbackEl)
	{
		const char* name = feedbackEl->Attribute("name");
		if (name)
			g_Skin.feedbackWindow.name = name;

		const char* title = feedbackEl->Attribute("title");
		if (title)
			g_Skin.feedbackWindow.title = ConvertUtf8ToWide(title);

		feedbackEl->QueryIntAttribute("width", &g_Skin.feedbackWindow.width);
		feedbackEl->QueryIntAttribute("height", &g_Skin.feedbackWindow.height);

		// Загружаем элементы окна обратной связи
		XMLElement* fbControlEl = feedbackEl->FirstChildElement("Control");
		while (fbControlEl)
		{
			UIControl control;

			const char* name = fbControlEl->Attribute("name");
			if (name)
				control.name = name;

			const char* type = fbControlEl->Attribute("type");
			if (type)
				control.type = ConvertStringToControlType(type);

			fbControlEl->QueryIntAttribute("id", &control.id);
			fbControlEl->QueryIntAttribute("x", &control.x);
			fbControlEl->QueryIntAttribute("y", &control.y);
			fbControlEl->QueryIntAttribute("width", &control.w);
			fbControlEl->QueryIntAttribute("height", &control.h);

			const char* text = fbControlEl->Attribute("text");
			if (text)
				control.text = ConvertUtf8ToWide(text);

			// Маппинг имен для окна обратной связи
			if (control.name == "feedback_edit")
				control.id = ID_EDIT_FB;
			else if (control.name == "send_button")
				control.id = ID_BTN_SEND_FB;
			else if (control.name == "cancel_button")
				control.id = ID_BTN_WARNING_NO;

			control.visible = true;
			control.enabled = true;

			g_Skin.feedbackWindow.controls.push_back(control);
			fbControlEl = fbControlEl->NextSiblingElement("Control");
		}
	}

	// Загружаем окно предупреждения для отзыва
	XMLElement* feedbackWarningEl = root->FirstChildElement("FeedbackWarningWindow");
	if (feedbackWarningEl)
	{
		const char* name = feedbackWarningEl->Attribute("name");
		if (name)
			g_Skin.feedbackWarningWindow.name = name;

		const char* title = feedbackWarningEl->Attribute("title");
		if (title)
			g_Skin.feedbackWarningWindow.title = ConvertUtf8ToWide(title);

		feedbackWarningEl->QueryIntAttribute("width", &g_Skin.feedbackWarningWindow.width);
		feedbackWarningEl->QueryIntAttribute("height", &g_Skin.feedbackWarningWindow.height);

		// Загружаем элементы окна предупреждения для отзыва
		XMLElement* warnControlEl = feedbackWarningEl->FirstChildElement("Control");
		while (warnControlEl)
		{
			UIControl control;

			const char* name = warnControlEl->Attribute("name");
			if (name)
				control.name = name;

			const char* type = warnControlEl->Attribute("type");
			if (type)
				control.type = ConvertStringToControlType(type);

			warnControlEl->QueryIntAttribute("id", &control.id);
			warnControlEl->QueryIntAttribute("x", &control.x);
			warnControlEl->QueryIntAttribute("y", &control.y);
			warnControlEl->QueryIntAttribute("width", &control.w);
			warnControlEl->QueryIntAttribute("height", &control.h);

			const char* text = warnControlEl->Attribute("text");
			if (text)
				control.text = ConvertUtf8ToWide(text);

			// Маппинг имен для окна предупреждения для отзыва
			if (control.name == "warning_label")
				control.id = 111;
			else if (control.name == "yes_button")
				control.id = ID_BTN_WARNING_YES;
			else if (control.name == "no_button")
				control.id = ID_BTN_WARNING_NO;

			control.visible = true;
			control.enabled = true;

			g_Skin.feedbackWarningWindow.controls.push_back(control);
			warnControlEl = warnControlEl->NextSiblingElement("Control");
		}
	}

	// Загружаем окно предупреждения для лога
	XMLElement* logWarningEl = root->FirstChildElement("LogWarningWindow");
	if (logWarningEl)
	{
		const char* name = logWarningEl->Attribute("name");
		if (name)
			g_Skin.logWarningWindow.name = name;

		const char* title = logWarningEl->Attribute("title");
		if (title)
			g_Skin.logWarningWindow.title = ConvertUtf8ToWide(title);

		logWarningEl->QueryIntAttribute("width", &g_Skin.logWarningWindow.width);
		logWarningEl->QueryIntAttribute("height", &g_Skin.logWarningWindow.height);

		// Загружаем элементы окна предупреждения для лога
		XMLElement* logWarnControlEl = logWarningEl->FirstChildElement("Control");
		while (logWarnControlEl)
		{
			UIControl control;

			const char* name = logWarnControlEl->Attribute("name");
			if (name)
				control.name = name;

			const char* type = logWarnControlEl->Attribute("type");
			if (type)
				control.type = ConvertStringToControlType(type);

			logWarnControlEl->QueryIntAttribute("id", &control.id);
			logWarnControlEl->QueryIntAttribute("x", &control.x);
			logWarnControlEl->QueryIntAttribute("y", &control.y);
			logWarnControlEl->QueryIntAttribute("width", &control.w);
			logWarnControlEl->QueryIntAttribute("height", &control.h);

			const char* text = logWarnControlEl->Attribute("text");
			if (text)
				control.text = ConvertUtf8ToWide(text);

			// Маппинг имен для окна предупреждения для лога
			if (control.name == "log_warning_label")
				control.id = 112;
			else if (control.name == "log_yes_button")
				control.id = ID_BTN_LOG_YES;
			else if (control.name == "log_no_button")
				control.id = ID_BTN_LOG_NO;

			control.visible = true;
			control.enabled = true;

			g_Skin.logWarningWindow.controls.push_back(control);
			logWarnControlEl = logWarnControlEl->NextSiblingElement("Control");
		}
	}

	// Устанавливаем значения по умолчанию, если не загружены из XML
	if (g_Skin.feedbackWindow.width == 0)
		g_Skin.feedbackWindow.width = 500;
	if (g_Skin.feedbackWindow.height == 0)
		g_Skin.feedbackWindow.height = 400;
	if (g_Skin.feedbackWindow.title.empty())
		g_Skin.feedbackWindow.title = L"Обратная связь";

	if (g_Skin.feedbackWarningWindow.width == 0)
		g_Skin.feedbackWarningWindow.width = 400;
	if (g_Skin.feedbackWarningWindow.height == 0)
		g_Skin.feedbackWarningWindow.height = 200;
	if (g_Skin.feedbackWarningWindow.title.empty())
		g_Skin.feedbackWarningWindow.title = L"Предупреждение";

	if (g_Skin.logWarningWindow.width == 0)
		g_Skin.logWarningWindow.width = 400;
	if (g_Skin.logWarningWindow.height == 0)
		g_Skin.logWarningWindow.height = 200;
	if (g_Skin.logWarningWindow.title.empty())
		g_Skin.logWarningWindow.title = L"Отправка лога";

	LogMessage(L"Конфигурация загружена успешно");
	LogMessage(L"Размеры окон:");
	LogMessage(L"  Главное окно: " + std::to_wstring(g_Skin.winW) + L"x" + std::to_wstring(g_Skin.winH));
	LogMessage(L"  Окно обратной связи: " + std::to_wstring(g_Skin.feedbackWindow.width) + L"x" +
			   std::to_wstring(g_Skin.feedbackWindow.height));
	LogMessage(L"  Окно предупреждения: " + std::to_wstring(g_Skin.feedbackWarningWindow.width) + L"x" +
			   std::to_wstring(g_Skin.feedbackWarningWindow.height));
	LogMessage(L"  Окно предупреждения лога: " + std::to_wstring(g_Skin.logWarningWindow.width) + L"x" +
			   std::to_wstring(g_Skin.logWarningWindow.height));
}

// --- СОЗДАНИЕ ОКОН ---
HWND CreateCenteredWindow(HWND hParent, const std::wstring& className, const std::wstring& title, int width, int height,
						  WNDPROC wndProc, bool registerClass)
{
	static std::map<std::wstring, bool> registeredClasses;

	if (registerClass && !registeredClasses[className])
	{
		WNDCLASSW wc = {0};
		wc.lpfnWndProc = wndProc;
		wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hParent, GWLP_HINSTANCE));
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wc.lpszClassName = className.c_str();
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		if (!RegisterClassW(&wc))
			return nullptr;

		registeredClasses[className] = true;
	}

	// Получаем размеры экрана
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Корректируем размеры, если они больше экрана
	if (width > screenWidth)
		width = screenWidth - 20;
	if (height > screenHeight)
		height = screenHeight - 20;

	// Получаем позицию родительского окна
	RECT parentRect;
	GetWindowRect(hParent, &parentRect);

	// Вычисляем позицию для центрирования
	int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
	int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

	// Корректируем позицию, чтобы окно не выходило за пределы экрана
	x = max(10, min(x, screenWidth - width - 10));
	y = max(10, min(y, screenHeight - height - 10));

	return CreateWindowW(className.c_str(), title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, x, y,
						 width, height, hParent, nullptr,
						 reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hParent, GWLP_HINSTANCE)),
						 reinterpret_cast<LPVOID>(hParent));
}

// --- ОКНО ПРЕДУПРЕЖДЕНИЯ ДЛЯ ЛОГА ---
LRESULT CALLBACK LogWarningWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont = nullptr;

	switch (message)
	{
	case WM_CREATE: {
		// Получаем родительское окно
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		HWND hParent = reinterpret_cast<HWND>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hParent));

		// Создаем шрифт
		hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

		// Создаем элементы управления
		for (const auto& control : g_Skin.logWarningWindow.controls)
		{
			if (!control.visible)
				continue;

			HWND hControl = nullptr;
			DWORD style = WS_CHILD | WS_VISIBLE;

			switch (control.type)
			{
			case UI_LABEL:
				style |= SS_CENTER;
				hControl = CreateWindowW(L"STATIC", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;

			case UI_BUTTON:
				style |= WS_TABSTOP;
				if (control.name == "log_yes_button")
					style |= BS_DEFPUSHBUTTON;
				else
					style |= BS_PUSHBUTTON;

				hControl = CreateWindowW(L"BUTTON", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;
			}

			if (hFont && hControl)
				SendMessage(hControl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
		}
		break;
	}

	case WM_COMMAND: {
		int cmdId = LOWORD(wParam);

		if (cmdId == ID_BTN_LOG_YES)
		{
			HWND hParent = reinterpret_cast<HWND>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			if (hParent)
			{
				DestroyWindow(hWnd);
				SendLogFile();
			}
		}
		else if (cmdId == ID_BTN_LOG_NO)
		{
			DestroyWindow(hWnd);
		}
		break;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (hFont)
			DeleteObject(hFont);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// --- ОКНО ПРЕДУПРЕЖДЕНИЯ ДЛЯ ОТЗЫВА ---
LRESULT CALLBACK FeedbackWarningWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont = nullptr;
	static HFONT hTitleFont = nullptr;

	switch (message)
	{
	case WM_CREATE: {
		// Получаем родительское окно
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		HWND hParent = reinterpret_cast<HWND>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hParent));

		// Создаем шрифты
		hTitleFont = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

		hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

		// Создаем элементы управления
		for (const auto& control : g_Skin.feedbackWarningWindow.controls)
		{
			if (!control.visible)
				continue;

			HWND hControl = nullptr;
			DWORD style = WS_CHILD | WS_VISIBLE;

			switch (control.type)
			{
			case UI_LABEL:
				style |= SS_LEFT;
				hControl = CreateWindowW(L"STATIC", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;

			case UI_BUTTON:
				style |= WS_TABSTOP;
				if (control.name == "yes_button")
					style |= BS_DEFPUSHBUTTON;
				else
					style |= BS_PUSHBUTTON;

				hControl = CreateWindowW(L"BUTTON", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;
			}

			if (hControl)
			{
				HFONT fontToUse = (control.name == "warning_label") ? hFont : hTitleFont;
				SendMessage(hControl, WM_SETFONT, reinterpret_cast<WPARAM>(fontToUse), TRUE);
			}
		}
		break;
	}

	case WM_COMMAND: {
		int cmdId = LOWORD(wParam);

		if (cmdId == ID_BTN_WARNING_YES)
		{
			HWND hParent = reinterpret_cast<HWND>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			if (hParent)
			{
				DestroyWindow(hWnd);
				OpenFeedbackForm(hParent);
			}
		}
		else if (cmdId == ID_BTN_WARNING_NO)
		{
			DestroyWindow(hWnd);
		}
		break;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (hFont)
			DeleteObject(hFont);
		if (hTitleFont)
			DeleteObject(hTitleFont);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// --- ОКНО ОБРАТНОЙ СВЯЗИ ---
LRESULT CALLBACK FeedbackWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit = nullptr;
	static HWND hSendButton = nullptr;
	static HWND hCancelButton = nullptr;
	static HFONT hFont = nullptr;
	static HWND hProgress = nullptr;

	switch (message)
	{
	case WM_CREATE: {
		// Создаем шрифт
		hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

		// Создаем элементы управления
		for (const auto& control : g_Skin.feedbackWindow.controls)
		{
			if (!control.visible)
				continue;

			HWND hControl = nullptr;
			DWORD style = WS_CHILD | WS_VISIBLE;

			switch (control.type)
			{
			case UI_EDIT:
				style |= WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER;
				hControl = CreateWindowW(L"EDIT", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				hEdit = hControl;
				SendMessage(hControl, EM_LIMITTEXT, 4096, 0);
				break;

			case UI_BUTTON:
				style |= WS_TABSTOP;
				if (control.name == "send_button")
				{
					style |= BS_DEFPUSHBUTTON;
					hSendButton = hControl;
				}
				else
				{
					style |= BS_PUSHBUTTON;
					if (control.name == "cancel_button")
						hCancelButton = hControl;
				}

				hControl = CreateWindowW(L"BUTTON", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;

			case UI_LABEL:
				hControl = CreateWindowW(L"STATIC", control.text.c_str(), style | SS_LEFT, control.x, control.y,
										 control.w, control.h, hWnd, nullptr, nullptr, nullptr);
				break;
			}

			if (hFont && hControl)
				SendMessage(hControl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
		}

		// Создаем индикатор прогресса
		hProgress = CreateWindowW(PROGRESS_CLASS, nullptr, WS_CHILD | PBS_MARQUEE, 10, 200, 300, 20, hWnd, nullptr,
								  nullptr, nullptr);
		if (hProgress)
		{
			SendMessage(hProgress, PBM_SETMARQUEE, TRUE, 0);
			ShowWindow(hProgress, SW_HIDE);
		}

		break;
	}

	case WM_COMMAND: {
		int cmdId = LOWORD(wParam);

		if (cmdId == ID_BTN_SEND_FB && !g_IsSendingFeedback)
		{
			// Проверяем наличие текста
			int textLength = GetWindowTextLengthW(hEdit);
			if (textLength == 0)
			{
				MessageBoxW(hWnd, L"Пожалуйста, напишите ваш отзыв перед отправкой.", L"Внимание",
							MB_OK | MB_ICONINFORMATION);
				break;
			}

			// Читаем текст
			std::vector<wchar_t> buffer(textLength + 1);
			GetWindowTextW(hEdit, &buffer[0], textLength + 1);
			std::wstring feedbackText = &buffer[0];

			// Сохраняем во временный файл
			std::wstring tempFilePath = JoinPaths(g_LauncherDir, L"feedback_temp.txt");
			std::string utf8Text = ConvertWideToUtf8(feedbackText);

			std::ofstream file(tempFilePath, std::ios::binary);
			if (!file.is_open())
			{
				MessageBoxW(hWnd, L"Не удалось создать временный файл", L"Ошибка", MB_OK | MB_ICONERROR);
				break;
			}

			file.write(utf8Text.c_str(), utf8Text.size());
			file.close();

			// Блокируем элементы управления
			EnableWindow(hEdit, FALSE);
			EnableWindow(hSendButton, FALSE);
			EnableWindow(hCancelButton, FALSE);

			// Показываем индикатор прогресса
			if (hProgress)
				ShowWindow(hProgress, SW_SHOW);

			// Устанавливаем курсор ожидания
			SetCursor(LoadCursor(nullptr, IDC_WAIT));

			// Запускаем асинхронную отправку
			SendFeedbackAsync(tempFilePath, g_EmailSubjects.feedbackSubject, hWnd);
		}
		else if (cmdId == ID_BTN_WARNING_NO && !g_IsSendingFeedback)
		{
			DestroyWindow(hWnd);
		}
		break;
	}

	case WM_FEEDBACK_SENT: {
		LogMessage(L"Обработка WM_FEEDBACK_SENT");

		// Восстанавливаем элементы управления
		EnableWindow(hEdit, TRUE);
		EnableWindow(hSendButton, TRUE);
		EnableWindow(hCancelButton, TRUE);

		// Скрываем индикатор прогресса
		if (hProgress)
			ShowWindow(hProgress, SW_HIDE);

		// Восстанавливаем курсор
		SetCursor(LoadCursor(nullptr, IDC_ARROW));

		// Сбрасываем флаги
		g_IsSendingFeedback = false;
		if (g_hCurrentFeedbackWnd == hWnd)
			g_hCurrentFeedbackWnd = nullptr;

		int result = static_cast<int>(wParam);
		if (result == 0)
		{
			MessageBoxW(hWnd, L"Спасибо! Ваш отзыв отправлен.", L"Успешно", MB_OK | MB_ICONINFORMATION);
			DestroyWindow(hWnd);
		}
		else
		{
			MessageBoxW(hWnd, L"Не удалось отправить отзыв. Проверьте подключение к интернету.", L"Ошибка",
						MB_OK | MB_ICONERROR);
		}
		break;
	}

	case WM_CLOSE:
		if (g_IsSendingFeedback)
		{
			MessageBoxW(hWnd, L"Пожалуйста, дождитесь завершения отправки.", L"Отправка в процессе",
						MB_OK | MB_ICONINFORMATION);
			return 0;
		}
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (hFont)
			DeleteObject(hFont);

		// Сбрасываем флаги
		g_IsSendingFeedback = false;
		if (g_hCurrentFeedbackWnd == hWnd)
			g_hCurrentFeedbackWnd = nullptr;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// --- ГЛАВНОЕ ОКНО ---
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE: {
		// Создаем шрифт
		HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

		// Создаем элементы управления
		for (const auto& control : g_Skin.mainWindowControls)
		{
			if (!control.visible)
				continue;

			HWND hControl = nullptr;
			DWORD style = WS_CHILD | WS_VISIBLE;

			switch (control.type)
			{
			case UI_BUTTON:
				style |= WS_TABSTOP;
				if (control.id == ID_BTN_PLAY)
					style |= BS_DEFPUSHBUTTON;
				else
					style |= BS_PUSHBUTTON;

				hControl = CreateWindowW(L"BUTTON", control.text.c_str(), style, control.x, control.y, control.w,
										 control.h, hWnd, reinterpret_cast<HMENU>(control.id), nullptr, nullptr);
				break;

			case UI_LABEL:
				hControl = CreateWindowW(L"STATIC", control.text.c_str(), style | SS_LEFT, control.x, control.y,
										 control.w, control.h, hWnd, nullptr, nullptr, nullptr);
				break;
			}

			if (hFont && hControl)
			{
				SendMessage(hControl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
				EnableWindow(hControl, control.enabled);
			}
		}

		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hFont));

		std::thread([hWnd]() {
			// Ждем 1 секунду, чтобы лаунчер успел появиться и прогрузиться
			std::this_thread::sleep_for(std::chrono::seconds(1));
			// Запускаем проверку в тихом режиме
			CheckForUpdates(hWnd, true);
		}).detach();

		break;
	}

	case WM_LBUTTONDOWN: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		POINT pt = {x, y};
		HWND hChild = ChildWindowFromPoint(hWnd, pt);

		// Если клик был не по кнопке, начинаем перетаскивание
		if (!hChild || hChild == hWnd)
		{
			g_DragStart.x = x;
			g_DragStart.y = y;
			SetCapture(hWnd);
		}
		break;
	}

	case WM_MOUSEMOVE:
		if (GetCapture() == hWnd)
		{
			POINT pt;
			GetCursorPos(&pt);

			RECT rcWindow;
			GetWindowRect(hWnd, &rcWindow);

			int newX = rcWindow.left + (pt.x - rcWindow.left - g_DragStart.x);
			int newY = rcWindow.top + (pt.y - rcWindow.top - g_DragStart.y);

			SetWindowPos(hWnd, nullptr, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		break;

	case WM_LBUTTONUP:
		if (GetCapture() == hWnd)
			ReleaseCapture();
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		if (g_hBgImage)
		{
			HDC hMemDC = CreateCompatibleDC(hdc);
			HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hMemDC, g_hBgImage));

			BITMAP bm;
			GetObject(g_hBgImage, sizeof(bm), &bm);

			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			SetStretchBltMode(hdc, HALFTONE);
			StretchBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

			SelectObject(hMemDC, hOldBitmap);
			DeleteDC(hMemDC);
		}
		else
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
		}

		EndPaint(hWnd, &ps);
		break;
	}

	case WM_COMMAND: {
		int cmdId = LOWORD(wParam);

		switch (cmdId)
		{
		case ID_BTN_PLAY:
			RunGame();
			break;

		case ID_BTN_BENCH:
			RunGame(BENCHMARK_ARGS);
			break;

		case ID_BTN_LOG:
			OpenLogWarningWindow(hWnd);
			break;

		case ID_BTN_FEEDBACK:
			OpenFeedbackWarning(hWnd);
			break;

		case ID_BTN_UPDATE:
			CheckForUpdates(hWnd);
			break;

		case ID_BTN_CLOSE:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY: {
		HFONT hFont = reinterpret_cast<HFONT>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (hFont)
			DeleteObject(hFont);

		if (g_hBgImage)
			DeleteObject(g_hBgImage);

		PostQuitMessage(0);
		break;
	}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// --- ИНИЦИАЛИЗАЦИЯ ПУТЕЙ ---
void InitializePaths()
{
	g_LauncherDir = GetExeDir();
	g_BinDir = JoinPaths(g_LauncherDir, BIN_DIR_REL);
	g_GamePath = JoinPaths(g_BinDir, GAME_EXE_NAME);
	g_SenderPath = JoinPaths(g_BinDir, LOG_SENDER_REL);
	g_UpdateServicePath = JoinPaths(g_LauncherDir, UPDATE_SERVICE_REL);
	g_LauncherResourcesDir = JoinPaths(g_BinDir, L"launcher");
}

// --- ЗАПУСК ИГРЫ ---
void RunGame(const std::wstring& args)
{
	LogMessage(L"Запуск игры: " + g_GamePath);

	if (!FileExists(g_GamePath))
	{
		MessageBoxW(nullptr, (L"Файл игры не найден:\n" + g_GamePath).c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	std::wstring commandLine = L"\"" + g_GamePath + L"\" " + args;
	ExecuteProcess(commandLine, g_BinDir, true);
}

// --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ПАРСИНГА ---

// Удаление пробелов с начала и конца строки
std::wstring Trim(const std::wstring& str)
{
	size_t first = str.find_first_not_of(L" \t\r\n");
	if (std::wstring::npos == first)
		return L"";
	size_t last = str.find_last_not_of(L" \t\r\n");
	return str.substr(first, (last - first + 1));
}

// Разделение строки по разделителю
std::vector<std::wstring> SplitString(const std::wstring& str, wchar_t delimiter)
{
	std::vector<std::wstring> tokens;
	std::wstring token;
	std::wstringstream ss(str);
	while (std::getline(ss, token, delimiter))
	{
		tokens.push_back(Trim(token));
	}
	return tokens;
}

// Структура для хранения записи LTX пути
struct FSEntry
{
	std::wstring parent; // Например: $app_data_root$
	std::wstring path;	 // Например: logs/
};

// Рекурсивное разрешение пути
std::wstring ResolveFSPath(const std::wstring& variable, const std::map<std::wstring, FSEntry>& fsMap)
{
	// Базовый случай: корень файловой системы (сама папка лаунчера)
	if (variable == L"$fs_root$")
	{
		return g_LauncherDir;
	}

	auto it = fsMap.find(variable);
	if (it == fsMap.end())
	{
		LogMessage(L"Не удалось найти переменную пути: " + variable);
		return L"";
	}

	// Рекурсивно ищем родителя
	std::wstring parentPath = ResolveFSPath(it->second.parent, fsMap);

	// Объединяем родительский путь и текущий подкаталог
	return JoinPaths(parentPath, it->second.path);
}

// Основная функция получения пути к папке логов
std::wstring GetLogDirFromConfig()
{
	std::wstring fsConfigPath = JoinPaths(g_LauncherDir, FS_LTX_NAME);

	if (!FileExists(fsConfigPath))
	{
		LogMessage(L"Конфигурационный файл не найден: " + fsConfigPath);
		return L"";
	}

	std::wifstream file(fsConfigPath);
	// Учитываем, что LTX обычно в ANSI/Windows-1251, но ключи у нас на английском, так что std::wstring считает
	// нормально Если будут проблемы с чтением, можно использовать бинарное чтение и конвертацию, но для структуры
	// $key$ = val|val|val|val это обычно не требуется.

	std::map<std::wstring, FSEntry> fsMap;
	std::wstring line;

	while (std::getline(file, line))
	{
		// Убираем комментарии (если есть ;)
		size_t commentPos = line.find(L';');
		if (commentPos != std::wstring::npos)
			line = line.substr(0, commentPos);

		line = Trim(line);
		if (line.empty())
			continue;

		// Ищем разделитель '='
		size_t eqPos = line.find(L'=');
		if (eqPos == std::wstring::npos)
			continue;

		std::wstring key = Trim(line.substr(0, eqPos));
		std::wstring valuesStr = line.substr(eqPos + 1);

		// Парсим значения, разделенные '|'
		// Формат: flags | flags | parent | path
		std::vector<std::wstring> parts = SplitString(valuesStr, L'|');

		if (parts.size() >= 3) // Нам нужно как минимум 3 части (или 4, если считать пустой хвост)
		{
			FSEntry entry;
			// X-Ray формат: $key$ = bool| bool| $parent$| path
			// parts[0] - bool
			// parts[1] - bool
			// parts[2] - parent ($fs_root$, $app_data_root$ и т.д.)
			// parts[3] - path (logs\, savedgames\ и т.д.) - может отсутствовать, если это корень

			entry.parent = parts[2];
			if (parts.size() > 3)
				entry.path = parts[3];
			else
				entry.path = L"";

			fsMap[key] = entry;
		}
	}
	file.close();

	// Теперь разрешаем путь для $logs$
	// Если в конфиге он называется иначе, поменяй строку ниже
	return ResolveFSPath(L"$logs$", fsMap);
}

// --- ОТПРАВКА ЛОГА ---
void SendLogFile()
{
	LogMessage(L"Подготовка к отправке лога...");

	// 1. Проверяем наличие MailService
	if (!FileExists(g_SenderPath))
	{
		MessageBoxW(nullptr, (L"MailService не найден:\n" + g_SenderPath).c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	// 2. Получаем путь к папке логов из game_filesystem.ltx
	std::wstring logsDir = GetLogDirFromConfig();
	std::wstring fullLogPath = L"";

	if (!logsDir.empty())
	{
		// Собираем полный путь: папка из конфига + постоянное имя файла
		fullLogPath = JoinPaths(logsDir, LOG_FILE_NAME);
		LogMessage(L"Ожидаемый путь к логу: " + fullLogPath);
	}
	else
	{
		LogMessage(L"Не удалось распарсить путь к логам из " + FS_LTX_NAME);
	}

	// 3. Проверяем, существует ли этот файл
	bool logFound = false;
	if (!fullLogPath.empty() && FileExists(fullLogPath))
	{
		logFound = true;
	}
	else
	{
		// Fallback: Если файла нет там, где сказано в конфиге, попробуем поискать "xray_engine.log" в папке
		// binaries/appdata/logs/ как резерв
		LogMessage(L"Лог по пути из конфига не найден. Пробуем стандартный путь...");
		std::wstring fallbackPath = JoinPaths(JoinPaths(g_BinDir, L"appdata\\logs"), LOG_FILE_NAME);

		if (FileExists(fallbackPath))
		{
			fullLogPath = fallbackPath;
			logFound = true;
			LogMessage(L"Найден лог по стандартному пути: " + fullLogPath);
		}
	}

	// Если всё ещё не нашли, шлём лог лаунчера или выдаем ошибку
	if (!logFound)
	{
		LogMessage(L"Файл игры " + LOG_FILE_NAME + L" не найден нигде.");

		std::wstring launcherLog = JoinPaths(g_LauncherDir, L"launcher_log.txt");
		if (FileExists(launcherLog))
		{
			int res = MessageBoxW(
				nullptr,
				(L"Файл лога игры (" + LOG_FILE_NAME + L") не найден.\nОтправить лог лаунчера вместо него?").c_str(),
				L"Лог не найден", MB_YESNO | MB_ICONWARNING);

			if (res == IDYES)
			{
				fullLogPath = launcherLog;
			}
			else
			{
				return;
			}
		}
		else
		{
			MessageBoxW(nullptr, L"Файлы логов отсутствуют.", L"Ошибка", MB_OK | MB_ICONERROR);
			return;
		}
	}

	std::wstring senderDir = g_SenderPath.substr(0, g_SenderPath.find_last_of(L"\\/"));

	// Формируем команду с кавычками вокруг пути
	std::wstring command = L"--debug --subject=\"" + g_EmailSubjects.logSubject + L"\" \"" + fullLogPath + L"\"";

	LogMessage(L"Запуск MailService: " + command);

	ExecuteProcess(L"\"" + g_SenderPath + L"\" " + command, senderDir, false);

	MessageBoxW(nullptr, L"Диагностические данные отправлены.", L"Успешно", MB_OK | MB_ICONINFORMATION);
}

// --- ОТКРЫТИЕ ОКОН ---
void OpenFeedbackWarning(HWND hParent)
{
	HWND hWarning = CreateCenteredWindow(hParent, L"FeedbackWarningWindowClass", g_Skin.feedbackWarningWindow.title,
										 g_Skin.feedbackWarningWindow.width, g_Skin.feedbackWarningWindow.height,
										 FeedbackWarningWndProc, true);

	if (hWarning)
		SetWindowPos(hWarning, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void OpenFeedbackForm(HWND hParent)
{
	HWND hFeedback =
		CreateCenteredWindow(hParent, L"FeedbackWindowClass", g_Skin.feedbackWindow.title, g_Skin.feedbackWindow.width,
							 g_Skin.feedbackWindow.height, FeedbackWndProc, true);

	if (hFeedback)
		SetWindowPos(hFeedback, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void OpenLogWarningWindow(HWND hParent)
{
	HWND hWarning =
		CreateCenteredWindow(hParent, L"LogWarningWindowClass", g_Skin.logWarningWindow.title,
							 g_Skin.logWarningWindow.width, g_Skin.logWarningWindow.height, LogWarningWndProc, true);

	if (hWarning)
		SetWindowPos(hWarning, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

// --- АСИНХРОННАЯ ОТПРАВКА ОТЗЫВА ---
void SendFeedbackAsync(const std::wstring& filePath, const std::wstring& subject, HWND hWnd)
{
	LogMessage(L"Начало асинхронной отправки отзыва");

	g_IsSendingFeedback = true;
	g_hCurrentFeedbackWnd = hWnd;

	// ВАЖНО: Копируем путь, так как он нужен в лямбде
	std::wstring safeFilePath = filePath;

	std::thread([safeFilePath, subject, hWnd]() {
		LogMessage(L"Поток отправки отзыва запущен");

		std::wstring senderDir = g_SenderPath.substr(0, g_SenderPath.find_last_of(L"\\/"));

		// ИСПРАВЛЕНИЕ: Добавляем путь к файлу в конец команды!
		// Обрати внимание на пробел перед кавычкой пути
		std::wstring command = L"--debug --subject=\"" + subject + L"\" \"" + safeFilePath + L"\"";
		std::wstring fullCommand = L"\"" + g_SenderPath + L"\" " + command;

		LogMessage(L"Запуск команды: " + fullCommand); // Логируем, что запускаем

		int result = ExecuteProcess(fullCommand, senderDir, false);

		LogMessage(L"Отправка завершена с кодом: " + std::to_wstring(result));

		// Удаляем временный файл
		_wremove(safeFilePath.c_str());

		// Отправляем сообщение в окно
		if (IsWindow(hWnd))
		{
			PostMessage(hWnd, WM_FEEDBACK_SENT, result, 0);
		}

		g_IsSendingFeedback = false;
		g_hCurrentFeedbackWnd = nullptr;
	}).detach();
}

// --- СИСТЕМА ОБНОВЛЕНИЙ ---
bool CheckUpdateDependencies()
{
	if (!FileExists(g_UpdateServicePath))
	{
		LogMessage(L"UpdateService не найден: " + g_UpdateServicePath);
		return false;
	}

	std::wstring updateDir = g_UpdateServicePath.substr(0, g_UpdateServicePath.find_last_of(L"\\/"));

	// Проверяем наличие curl.exe
	std::wstring curlPath = JoinPaths(updateDir, L"curl.exe");
	if (!FileExists(curlPath))
	{
		LogMessage(L"curl.exe не найден: " + curlPath);
	}

	// Проверяем наличие 7za.exe
	std::wstring sevenZipPath = JoinPaths(updateDir, L"7za.exe");
	if (!FileExists(sevenZipPath))
	{
		MessageBoxW(nullptr,
					L"Для работы обновлений требуется 7za.exe\n"
					L"Поместите его в папку с UpdateService",
					L"Внимание", MB_OK | MB_ICONINFORMATION);
		return false;
	}

	return true;
}

void CheckForUpdates(HWND hWnd, bool silent)
{
	LogMessage(L"Проверка обновлений" + std::wstring(silent ? L" (тихий режим)" : L""));

	// В тихом режиме мы не спамим ошибками зависимостей, просто пишем в лог
	if (!CheckUpdateDependencies())
	{
		if (!silent)
		{
			MessageBoxW(hWnd, L"Система обновлений не настроена правильно.", L"Ошибка", MB_OK | MB_ICONERROR);
		}
		else
		{
			LogMessage(L"Ошибка: зависимости обновлений не найдены при автопроверке.");
		}
		return;
	}

	std::wstring updateDir = g_UpdateServicePath.substr(0, g_UpdateServicePath.find_last_of(L"\\/"));
	std::wstring checkCommand = L"\"" + g_UpdateServicePath + L"\" --check --work-dir \"" + g_LauncherDir + L"\"";

	// Выполняем проверку
	int result = ExecuteProcess(checkCommand, updateDir, false);

	switch (result)
	{
	case 1: // Обновление доступно
	{
		// Показываем сообщение даже в тихом режиме, так как это важно
		int choice = MessageBoxW(hWnd,
								 L"Доступно обновление!\n"
								 L"Установить сейчас?",
								 L"Обновление", MB_YESNO | MB_ICONQUESTION);

		if (choice == IDYES)
		{
			// Если пользователь согласился, можно вывести предупреждение, если нужно
			if (!silent)
			{
				MessageBoxW(hWnd, L"Лаунчер закроется для установки обновлений.", L"Обновление",
							MB_OK | MB_ICONINFORMATION);
			}

			std::wstring updateCommand =
				L"\"" + g_UpdateServicePath + L"\" --update --work-dir \"" + g_LauncherDir + L"\"";

			// Запускаем процесс обновления. Важно: здесь мы не ждем завершения (ExecuteProcess может блокировать),
			// но так как мы скоро выходим, лучше использовать ShellExecute или CreateProcess без ожидания,
			// но твой текущий ExecuteProcess подойдет, если апдейтер сам быстро отпускает процесс.
			// Однако правильнее запустить апдейтер и закрыть лаунчер.

			// Запускаем апдейтер так, чтобы он работал после закрытия лаунчера
			ShellExecuteW(NULL, L"open", g_UpdateServicePath.c_str(),
						  (L"--update --work-dir \"" + g_LauncherDir + L"\"").c_str(), updateDir.c_str(), SW_SHOW);

			// Закрываем лаунчер
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	}

	case 0:			 // Обновлений нет
		if (!silent) // Показываем только если нажали кнопку
		{
			MessageBoxW(hWnd, L"Установлена последняя версия.", L"Обновлений нет", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			LogMessage(L"Автопроверка: обновлений не найдено.");
		}
		break;

	case -1: // Ошибка
	default:
		if (!silent) // Показываем ошибку только при ручном нажатии
		{
			MessageBoxW(hWnd,
						L"Не удалось проверить обновления.\n"
						L"Проверьте подключение к интернету.",
						L"Ошибка", MB_OK | MB_ICONWARNING);
		}
		else
		{
			LogMessage(L"Автопроверка: ошибка соединения или проверки.");
		}
		break;
	}
}

// ============================================
// ТОЧКА ВХОДА
// ============================================
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Инициализация консоли для отладки
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);

	// Удаляем старый лог
	_wremove(L"launcher_log.txt");
	LogMessage(L"=== ЛАУНЧЕР ЗАПУЩЕН ===");

	// Инициализация стилей
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&icex);

	// Инициализация путей
	InitializePaths();

	// Загрузка конфигурации
	std::wstring xmlPath = JoinPaths(g_LauncherResourcesDir, L"layout.xml");
	LoadSkinConfig(xmlPath);

	// Загрузка фонового изображения
	std::wstring bgPath = JoinPaths(g_LauncherResourcesDir, g_Skin.bgImage);
	g_hBgImage = reinterpret_cast<HBITMAP>(LoadImageW(nullptr, bgPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));

	if (!g_hBgImage)
	{
		LogMessage(L"Не удалось загрузить фоновое изображение: " + bgPath);
	}

	// Регистрация класса главного окна
	WNDCLASSEXW wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"LauncherClass";
	wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassExW(&wcex))
	{
		MessageBoxW(nullptr, L"Ошибка регистрации класса окна.", L"Критическая ошибка", MB_ICONERROR);
		return 1;
	}

	// Создание главного окна
	HWND hWnd = CreateWindowW(L"LauncherClass", g_Skin.title.c_str(), WS_POPUP, 100, 100, g_Skin.winW, g_Skin.winH,
							  nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		DWORD err = GetLastError();
		MessageBoxW(nullptr, (L"Не удалось создать окно. Код: " + std::to_wstring(err)).c_str(), L"Критическая ошибка",
					MB_ICONERROR);
		return 1;
	}

	// Показ окна
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Цикл сообщений
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Завершение
	FreeConsole();
	return static_cast<int>(msg.wParam);
}
