// Installer.cpp
// Компилировать как Windows Application
// Особенности: 7zr.exe ВШИТ ВНУТРЬ (нет лишних загрузок)

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>
#include <map>
#include "resource.h"

// Определяем флаги TLS вручную
#ifndef INTERNET_OPTION_SECURE_PROTOCOLS
#define INTERNET_OPTION_SECURE_PROTOCOLS 136
#endif
#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT 0x00000800
#endif
#ifndef INTERNET_FLAG_IGNORE_CERT_AUTHORITY_INVALID
#define INTERNET_FLAG_IGNORE_CERT_AUTHORITY_INVALID 0x00002000
#endif

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(                                                                                                       \
	linker,                                                                                                            \
	"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ==========================================
// КОНФИГУРАЦИЯ
// ==========================================

const std::wstring INFO_FILE_URL = L"https://drive.google.com/uc?export=download&id=1odFpotFS20y_uTfEQHxiKMTzSkgx6Tfu";
const std::wstring APP_TITLE = L"S.T.A.L.K.E.R: ReLighted Installer";

const std::wstring TEMP_CONFIG_NAME = L"install_data.txt";
const std::wstring TEMP_ARCHIVER_EXE = L"7zr.exe";
const std::wstring TEMP_ARCHIVE_NAME = L"game_archive.7z";

#define ID_EDIT_PATH 101
#define ID_BTN_BROWSE 102
#define ID_BTN_INSTALL 103
#define ID_PROGRESS 104
#define ID_STATUS_LABEL 105

HWND hEditPath, hBtnBrowse, hBtnInstall, hProgress, hStatusLabel, hMainWindow;
HFONT hFont;

// ==========================================
// ЛОГИРОВАНИЕ
// ==========================================

void Log(const std::wstring& msg)
{
	std::wstring output = L"[INSTALLER] " + msg + L"\n";
	OutputDebugStringW(output.c_str());
}

void LogError(const std::wstring& context, DWORD errorCode)
{
	std::wstring msg = context + L" Error Code: " + std::to_wstring(errorCode);
	Log(msg);
}

void SetStatus(const std::wstring& text)
{
	Log(L"Status: " + text);
	SetWindowTextW(hStatusLabel, text.c_str());
}

// ==========================================
// УТИЛИТЫ РЕСУРСОВ (ГЛАВНОЕ ИЗМЕНЕНИЕ)
// ==========================================

// Функция извлечения вшитого EXE во временную папку
bool ExtractResource(int resourceId, const std::wstring& outputPath)
{
	Log(L"Extracting internal resource ID: " + std::to_wstring(resourceId));

	// 1. Находим ресурс в памяти
	HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), L"BINARY");
	if (!hResource)
	{
		LogError(L"FindResource failed", GetLastError());
		return false;
	}

	// 2. Загружаем ресурс
	HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
	if (!hLoadedResource)
	{
		LogError(L"LoadResource failed", GetLastError());
		return false;
	}

	// 3. Получаем указатель на данные и размер
	LPVOID pLockedResource = LockResource(hLoadedResource);
	DWORD dwResourceSize = SizeofResource(NULL, hResource);
	if (!pLockedResource || dwResourceSize == 0)
	{
		LogError(L"LockResource/Size failed", GetLastError());
		return false;
	}

	// 4. Записываем на диск
	HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		LogError(L"CreateFile failed", GetLastError());
		return false;
	}

	DWORD dwBytesWritten;
	BOOL bWrite = WriteFile(hFile, pLockedResource, dwResourceSize, &dwBytesWritten, NULL);
	CloseHandle(hFile);

	if (!bWrite || dwBytesWritten != dwResourceSize)
	{
		LogError(L"WriteFile failed", GetLastError());
		return false;
	}

	Log(L"Resource extracted successfully to: " + outputPath);
	return true;
}

// ==========================================
// ОСТАЛЬНЫЕ УТИЛИТЫ
// ==========================================

void EnableSecureProtocols()
{
	DWORD dwProtocols = 0;
	const DWORD WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 = 0x00000200;
	const DWORD WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 = 0x00000800;
	dwProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	InternetSetOption(NULL, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL, &dwProtocols, sizeof(dwProtocols));
}

std::wstring ToWide(const std::string& str)
{
	if (str.empty())
		return L"";
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string Trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	if (std::string::npos == first)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

std::map<std::string, std::string> ParseConfig(const std::wstring& path)
{
	std::map<std::string, std::string> config;
	std::ifstream file(path);
	if (!file.is_open())
		return config;
	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#' || line[0] == '[')
			continue;
		size_t eqPos = line.find('=');
		if (eqPos != std::string::npos)
		{
			config[Trim(line.substr(0, eqPos))] = Trim(line.substr(eqPos + 1));
		}
	}
	return config;
}

// ==========================================
// СЕТЬ (WININET)
// ==========================================

bool DownloadFile(const std::wstring& url, const std::wstring& destPath)
{
	Log(L"Downloading: " + url);
	DeleteUrlCacheEntryW(url.c_str());

	HINTERNET hInternet =
		InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInternet)
		return false;

	DWORD secureProtocols = SP_PROT_TLS1_2_CLIENT | 0x00000200;
	InternetSetOption(hInternet, INTERNET_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));

	DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE |
				  INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
				  INTERNET_FLAG_IGNORE_CERT_AUTHORITY_INVALID;
	HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, flags, 0);
	if (!hUrl)
	{
		LogError(L"Net Error", GetLastError());
		InternetCloseHandle(hInternet);
		return false;
	}

	DWORD contentLength = 0;
	DWORD bufLen = sizeof(contentLength);
	DWORD index = 0;
	HttpQueryInfo(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &bufLen, &index);

	HANDLE hFile = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hInternet);
		return false;
	}

	const DWORD BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE];
	DWORD bytesRead, totalBytesRead = 0;
	bool success = true;

	while (true)
	{
		if (!InternetReadFile(hUrl, buffer, BUFFER_SIZE, &bytesRead))
		{
			success = false;
			break;
		}
		if (bytesRead == 0)
			break;
		DWORD bytesWritten;
		if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL))
		{
			success = false;
			break;
		}
		totalBytesRead += bytesRead;
		if (contentLength > 0)
		{
			int percent = (int)((double)totalBytesRead / contentLength * 100.0);
			PostMessage(hProgress, PBM_SETPOS, percent, 0);
		}
	}

	CloseHandle(hFile);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);
	return success;
}

// ==========================================
// РАСПАКОВКА
// ==========================================

int ExtractArchive(const std::wstring& archiverPath, const std::wstring& archivePath, const std::wstring& destDir)
{
	std::wstring cmd = L"\"" + archiverPath + L"\" x \"" + archivePath + L"\" -o\"" + destDir + L"\" -aoa -y";
	Log(L"Exec: " + cmd);

	STARTUPINFOW si = {0};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi = {0};
	std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
	cmdBuf.push_back(0);

	if (CreateProcessW(NULL, &cmdBuf[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode = 0;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return (int)exitCode;
	}
	return -1;
}

// ==========================================
// ПОТОК УСТАНОВКИ
// ==========================================

void InstallThreadFunc(std::wstring installPath)
{
	Log(L"Installation started.");
	CoInitialize(NULL);
	EnableSecureProtocols();
	SHCreateDirectoryExW(NULL, installPath.c_str(), NULL);

	wchar_t tempPathArr[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPathArr);
	std::wstring tempDir = tempPathArr;
	std::wstring configPath = tempDir + TEMP_CONFIG_NAME;
	std::wstring archiverPath = tempDir + TEMP_ARCHIVER_EXE;
	std::wstring archivePath = tempDir + TEMP_ARCHIVE_NAME;

	PostMessage(hProgress, PBM_SETPOS, 0, 0);

	// 1. ИЗВЛЕЧЕНИЕ АРХИВАТОРА ИЗ РЕСУРСОВ
	SetStatus(L"Подготовка инструментов...");
	// ВАЖНО: Замените IDR_BINARY1 на ваш ID из resource.h (например, IDR_7ZR)
	if (!ExtractResource(IDR_BINARY1, archiverPath))
	{
		MessageBoxW(hMainWindow, L"Ошибка внутреннего ресурса (7zr.exe).", L"Error", MB_ICONERROR);
		EnableWindow(hBtnInstall, TRUE);
		CoUninitialize();
		return;
	}

	// 2. КОНФИГ
	SetStatus(L"Получение информации...");
	if (!DownloadFile(INFO_FILE_URL, configPath))
	{
		MessageBoxW(hMainWindow, L"Ошибка сети (конфиг).", L"Error", MB_ICONERROR);
		EnableWindow(hBtnInstall, TRUE);
		CoUninitialize();
		return;
	}

	auto config = ParseConfig(configPath);
	if (config["url_game"].empty())
	{ // url_7z_exe больше не нужен!
		MessageBoxW(hMainWindow, L"Ошибка конфига (нет ссылки на игру).", L"Error", MB_ICONERROR);
		EnableWindow(hBtnInstall, TRUE);
		CoUninitialize();
		return;
	}

	// 3. ИГРА
	SetStatus(L"Скачивание игры...");
	if (!DownloadFile(ToWide(config["url_game"]), archivePath))
	{
		MessageBoxW(hMainWindow, L"Ошибка загрузки игры.", L"Error", MB_ICONERROR);
		EnableWindow(hBtnInstall, TRUE);
		CoUninitialize();
		return;
	}

	// 4. РАСПАКОВКА
	SetStatus(L"Распаковка...");
	PostMessage(hProgress, PBM_SETMARQUEE, TRUE, 50);
	SetWindowLong(hProgress, GWL_STYLE, GetWindowLong(hProgress, GWL_STYLE) | PBS_MARQUEE);

	int exitCode = ExtractArchive(archiverPath, archivePath, installPath);

	PostMessage(hProgress, PBM_SETMARQUEE, FALSE, 0);
	if (exitCode == 0)
	{
		PostMessage(hProgress, PBM_SETPOS, 100, 0);
		MessageBoxW(hMainWindow, L"Установка завершена!", L"Успех", MB_ICONINFORMATION);
	}
	else
	{
		MessageBoxW(hMainWindow, (L"Ошибка распаковки: " + std::to_wstring(exitCode)).c_str(), L"Error", MB_ICONERROR);
	}

	DeleteFileW(configPath.c_str());
	DeleteFileW(archiverPath.c_str());
	DeleteFileW(archivePath.c_str());
	CoUninitialize();
	EnableWindow(hBtnInstall, TRUE);
}

// ==========================================
// GUI (Стандартный)
// ==========================================
void BrowseFolder()
{
	BROWSEINFOW bi = {0};
	bi.lpszTitle = L"Папка";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.hwndOwner = hMainWindow;
	LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if (pidl != 0)
	{
		wchar_t path[MAX_PATH];
		if (SHGetPathFromIDListW(pidl, path))
			SetWindowTextW(hEditPath, path);
		CoTaskMemFree(pidl);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
		CreateWindowW(L"STATIC", L"Путь установки:", WS_VISIBLE | WS_CHILD, 20, 20, 120, 20, hWnd, NULL, NULL, NULL);
		hEditPath = CreateWindowW(L"EDIT", L"C:\\Games\\MyGame", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20,
								  45, 340, 25, hWnd, (HMENU)ID_EDIT_PATH, NULL, NULL);
		hBtnBrowse = CreateWindowW(L"BUTTON", L"...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 370, 45, 40, 25, hWnd,
								   (HMENU)ID_BTN_BROWSE, NULL, NULL);
		hBtnInstall = CreateWindowW(L"BUTTON", L"Установить", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 150, 90, 140,
									35, hWnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
		hStatusLabel = CreateWindowW(L"STATIC", L"Готов", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 140, 400, 20, hWnd,
									 (HMENU)ID_STATUS_LABEL, NULL, NULL);
		hProgress = CreateWindowW(PROGRESS_CLASS, NULL, WS_VISIBLE | WS_CHILD | PBS_SMOOTH, 20, 165, 400, 20, hWnd,
								  (HMENU)ID_PROGRESS, NULL, NULL);
		EnumChildWindows(
			hWnd,
			[](HWND h, LPARAM l) -> BOOL {
				SendMessage(h, WM_SETFONT, (WPARAM)l, TRUE);
				return TRUE;
			},
			(LPARAM)hFont);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BTN_BROWSE)
			BrowseFolder();
		else if (LOWORD(wParam) == ID_BTN_INSTALL)
		{
			int len = GetWindowTextLengthW(hEditPath);
			std::vector<wchar_t> buf(len + 1);
			GetWindowTextW(hEditPath, &buf[0], len + 1);
			std::wstring path = &buf[0];
			if (path.empty())
			{
				MessageBoxW(hWnd, L"Укажите путь!", L"Warning", MB_ICONWARNING);
				break;
			}
			EnableWindow(hBtnInstall, FALSE);
			std::thread(InstallThreadFunc, path).detach();
		}
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
	WNDCLASSEXW wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"InstRes";
	RegisterClassExW(&wcex);
	hMainWindow = CreateWindowW(L"InstRes", APP_TITLE.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
								(GetSystemMetrics(SM_CXSCREEN) - 450) / 2, (GetSystemMetrics(SM_CYSCREEN) - 240) / 2,
								450, 240, nullptr, nullptr, hInstance, nullptr);
	ShowWindow(hMainWindow, nCmdShow);
	UpdateWindow(hMainWindow);
	Log(L"=== Installer Started ===");
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
