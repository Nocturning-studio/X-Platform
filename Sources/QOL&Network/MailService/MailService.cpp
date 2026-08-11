// MailService.cpp
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <algorithm>
#include <thread>
#include <chrono>

// --- НАСТРОЙКИ ---
const std::string GMAIL_TO = "nocturninglogs@gmail.com";
std::string g_CustomSubject = "MailServiceDefaultMessage";
bool g_IsDebugMode = true;

// --- ТИПЫ ФУНКЦИЙ ИЗ DLL ---
typedef void (*GetCredsFunc)(char* buffer, int maxLen);

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
std::string g_GmailUser = "";
std::string g_GmailPass = "";

// --- ЛОГИРОВАНИЕ ---
void WriteDebugLog(const std::wstring& message)
{
	std::wofstream logFile(L"mailservice_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		logFile << L"[" << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"] " << message << std::endl;
		logFile.close();
	}
}

void PrintToConsole(const std::wstring& msg)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		std::wstring fullMsg = msg + L"\n";
		DWORD written;
		WriteConsoleW(hOut, fullMsg.c_str(), static_cast<DWORD>(fullMsg.length()), &written, NULL);
	}
}

// --- ЗАГРУЗКА КРЕДЕНЦИАЛОВ ---
bool LoadCredentials(const std::wstring& exeDir)
{
	WriteDebugLog(L"Загрузка учетных данных из AuthLib.dll...");

	std::wstring dllPath = exeDir + L"\\AuthLib.dll";
	WriteDebugLog(L"Путь к DLL: " + dllPath);

	HMODULE hDll = LoadLibraryW(dllPath.c_str());
	if (!hDll)
	{
		DWORD err = GetLastError();
		WriteDebugLog(L"Ошибка загрузки AuthLib.dll. Код: " + std::to_wstring(err));
		return false;
	}

	GetCredsFunc getUserFunc = (GetCredsFunc)GetProcAddress(hDll, "GetUser");
	GetCredsFunc getPassFunc = (GetCredsFunc)GetProcAddress(hDll, "GetPass");

	if (!getUserFunc || !getPassFunc)
	{
		WriteDebugLog(L"Функции DLL не найдены");
		FreeLibrary(hDll);
		return false;
	}

	char userBuf[256] = {0};
	char passBuf[256] = {0};

	getUserFunc(userBuf, 256);
	getPassFunc(passBuf, 256);

	g_GmailUser = std::string(userBuf);
	g_GmailPass = std::string(passBuf);

	FreeLibrary(hDll);

	// Очистка буферов
	SecureZeroMemory(userBuf, sizeof(userBuf));
	SecureZeroMemory(passBuf, sizeof(passBuf));

	WriteDebugLog(L"Учетные данные загружены");
	return true;
}

// --- КОНВЕРТАЦИЯ СТРОК ---
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

// --- ФАЙЛОВЫЕ ОПЕРАЦИИ ---
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

	std::wstring cleanPart2 = part2;
	while (!cleanPart2.empty() && (cleanPart2[0] == L'\\' || cleanPart2[0] == L'/'))
		cleanPart2.erase(0, 1);

	return result + cleanPart2;
}

bool FileExists(const std::wstring& filepath)
{
	DWORD fileAttr = GetFileAttributesW(filepath.c_str());
	return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

std::string ReadFileContent(const std::wstring& filepath)
{
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return "";

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string content(size, 0);
	file.read(&content[0], size);
	return content;
}

// --- ЗАПУСК ПРОЦЕССА С ТАЙМАУТОМ ---
int ExecuteProcessWithTimeout(const std::wstring& cmdLine, const std::wstring& workDir, bool showWindow,
							  DWORD timeoutMs = 60000)
{
	WriteDebugLog(L"Запуск процесса: " + cmdLine);

	STARTUPINFOW si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);

	if (!showWindow)
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	// Создаем копию строки команды
	std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
	cmdBuffer.push_back(0);

	// Создаем рабочую директорию
	std::vector<wchar_t> workDirBuffer;
	const wchar_t* workDirPtr = nullptr;

	if (!workDir.empty())
	{
		workDirBuffer = std::vector<wchar_t>(workDir.begin(), workDir.end());
		workDirBuffer.push_back(0);
		workDirPtr = &workDirBuffer[0];
	}

	// Создаем процесс
	if (!CreateProcessW(nullptr, &cmdBuffer[0], nullptr, nullptr, FALSE, 0, nullptr, workDirPtr, &si, &pi))
	{
		DWORD err = GetLastError();
		WriteDebugLog(L"Ошибка CreateProcess: " + std::to_wstring(err));
		return -1;
	}

	// Ждем завершения с таймаутом
	DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
	DWORD exitCode = 0;

	if (waitResult == WAIT_TIMEOUT)
	{
		WriteDebugLog(L"Таймаут процесса, завершение...");
		TerminateProcess(pi.hProcess, 1);
		exitCode = 1;
	}
	else if (waitResult == WAIT_OBJECT_0)
	{
		GetExitCodeProcess(pi.hProcess, &exitCode);
	}
	else
	{
		WriteDebugLog(L"Ошибка ожидания процесса");
		TerminateProcess(pi.hProcess, 1);
		exitCode = 1;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	WriteDebugLog(L"Процесс завершен с кодом: " + std::to_wstring(exitCode));
	return static_cast<int>(exitCode);
}

// --- ОСНОВНАЯ ФУНКЦИЯ ОТПРАВКИ ---
int SendEmail(const std::wstring& inputFile, const std::wstring& subject)
{
	WriteDebugLog(L"Начало отправки письма");

	std::wstring exeDir = GetExeDir();

	// Проверяем наличие curl.exe
	std::wstring curlPath = JoinPaths(exeDir, L"curl.exe");
	if (!FileExists(curlPath))
	{
		WriteDebugLog(L"curl.exe не найден: " + curlPath);
		return -1;
	}

	// Проверяем наличие сертификата
	std::wstring certPath = JoinPaths(exeDir, L"cacert.pem");
	if (!FileExists(certPath))
	{
		WriteDebugLog(L"cacert.pem не найден: " + certPath);
		return -1;
	}

	// Загружаем содержимое файла
	std::string fileContent = ReadFileContent(inputFile);
	if (fileContent.empty())
	{
		WriteDebugLog(L"Файл пуст или не может быть прочитан");
		return -1;
	}

	// Создаем временный файл для письма
	std::wstring tempFilePath = JoinPaths(exeDir, L"mail_temp.txt");

	std::ofstream mailFile(tempFilePath, std::ios::binary);
	if (!mailFile.is_open())
	{
		WriteDebugLog(L"Не удалось создать временный файл");
		return -1;
	}

	// Конвертируем тему письма из wstring в UTF-8 string
	std::string subjectUtf8 = ConvertWideToUtf8(subject);

	// Формируем заголовки письма
	mailFile << "From: " << g_GmailUser << "\r\n";
	mailFile << "To: " << GMAIL_TO << "\r\n";
	mailFile << "Subject: " << subjectUtf8 << "\r\n"; // Используем UTF-8 строку
	mailFile << "MIME-Version: 1.0\r\n";
	mailFile << "Content-Type: text/plain; charset=UTF-8\r\n";
	mailFile << "Content-Transfer-Encoding: 8bit\r\n";
	mailFile << "\r\n";

	// Записываем тело письма
	mailFile << fileContent;
	mailFile.close();

	// Формируем команду curl с таймаутами
	std::wstringstream cmd;
	cmd << L"\"" << curlPath << L"\" ";

	if (g_IsDebugMode)
		cmd << L"-v ";

	// Критически важные параметры для предотвращения зависаний
	cmd << L"--max-time 120 ";		 // Максимальное время выполнения: 2 минуты
	cmd << L"--connect-timeout 30 "; // Таймаут соединения: 30 секунд
	cmd << L"--retry 0 ";			 // Не повторять при ошибках

	cmd << L"--user \"" << ConvertAnsiToWide(g_GmailUser) << L":" << ConvertAnsiToWide(g_GmailPass) << L"\" ";
	cmd << L"--url \"smtp://smtp.gmail.com:587\" ";
	cmd << L"--ssl-reqd ";
	cmd << L"--cacert \"" << certPath << L"\" ";
	cmd << L"--mail-from \"" << ConvertAnsiToWide(g_GmailUser) << L"\" ";
	cmd << L"--mail-rcpt \"" << ConvertAnsiToWide(GMAIL_TO) << L"\" ";
	cmd << L"--upload-file \"" << tempFilePath << L"\" ";
	cmd << L"--login-options AUTH=LOGIN ";

	std::wstring curlCommand = cmd.str();
	WriteDebugLog(L"Команда curl: " + curlCommand);

	// Запускаем curl с таймаутом
	int result = ExecuteProcessWithTimeout(curlCommand, exeDir, false, 180000); // 3 минуты

	// Удаляем временный файл
	_wremove(tempFilePath.c_str());

	// Очищаем чувствительные данные
	g_GmailPass.assign(g_GmailPass.length(), 'X');
	g_GmailUser.assign(g_GmailUser.length(), 'X');

	return result;
}

// --- ОБРАБОТКА АРГУМЕНТОВ КОМАНДНОЙ СТРОКИ ---
struct CommandLineArgs
{
	std::wstring inputFile;
	std::wstring subject;
	bool debugMode = false;
};

CommandLineArgs ParseCommandLine(int argc, wchar_t* argv[])
{
	CommandLineArgs args;

	for (int i = 1; i < argc; i++)
	{
		std::wstring arg = argv[i];

		if (arg == L"--debug")
		{
			args.debugMode = true;
			g_IsDebugMode = true;
			WriteDebugLog(L"Режим отладки включен");
		}
		else if (arg.find(L"--subject=") == 0)
		{
			args.subject = arg.substr(10); // Убираем "--subject="
			WriteDebugLog(L"Тема письма: " + args.subject);
		}
		else if (args.inputFile.empty() && arg.substr(0, 2) != L"--")
		{
			args.inputFile = arg;
			WriteDebugLog(L"Входной файл: " + args.inputFile);
		}
	}

	args.debugMode = true;
	g_IsDebugMode = true;
	WriteDebugLog(L"Режим отладки включен");

	return args;
}

// --- ТОЧКА ВХОДА ---
int wmain(int argc, wchar_t* argv[])
{
	// ИСПРАВЛЕНИЕ 1: Установка локали для корректной записи кириллицы в файл
	std::locale::global(std::locale(""));

	// Очищаем старый лог
	_wremove(L"mailservice_log.txt");
	WriteDebugLog(L"=== MailService запущен ===");

	// Парсим аргументы
	CommandLineArgs args = ParseCommandLine(argc, argv);

	// Инициализация консоли для отладки
	if (args.debugMode)
	{
		AllocConsole();

		FILE* f;
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);

		PrintToConsole(L"MailService Debug Mode Active");
		PrintToConsole(L"Arguments count: " + std::to_wstring(argc));

		for (int i = 0; i < argc; ++i)
		{
			PrintToConsole(L"Arg[" + std::to_wstring(i) + L"]: " + argv[i]);
		}
	}

	// Получаем директорию исполняемого файла
	std::wstring exeDir = GetExeDir();

	// Загружаем учетные данные
	if (!LoadCredentials(exeDir))
	{
		WriteDebugLog(L"Не удалось загрузить учетные данные");
		if (args.debugMode)
		{
			std::cout << "Ошибка загрузки учетных данных (DLL fail)\n";
		}
		return 1;
	}

	// Проверяем входной файл
	if (args.inputFile.empty() || !FileExists(args.inputFile))
	{
		WriteDebugLog(L"Входной файл не указан или не существует: " + args.inputFile);
		if (args.debugMode)
		{
			// Используем wcout для вывода пути, чтобы не было проблем с кодировкой пути
			std::wcout << L"Ошибка: входной файл не найден или пуст путь." << std::endl;
			std::wcout << L"Ожидался путь: [" << args.inputFile << L"]" << std::endl;
		}
		return 2;
	}

	// Преобразуем тему письма
	std::string subjectUtf8;
	if (!args.subject.empty())
	{
		subjectUtf8 = ConvertWideToUtf8(args.subject);
		std::replace(subjectUtf8.begin(), subjectUtf8.end(), '-', ' ');
	}
	else
	{
		subjectUtf8 = g_CustomSubject;
	}

	// Отправляем письмо
	int result = SendEmail(args.inputFile, ConvertAnsiToWide(subjectUtf8));

	WriteDebugLog(L"MailService завершен с кодом: " + std::to_wstring(result));

	if (args.debugMode)
	{
		std::cout << "\nРезультат curl: " << result << std::endl;
		FreeConsole();
	}

	return result;
}
