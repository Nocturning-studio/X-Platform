// UpdateService.cpp
// Компиляция: g++ UpdateService.cpp -o UpdateService.exe -static
// Требования: curl.exe и 7za.exe должны лежать рядом с бинарником для работы обновления.

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <codecvt>
#include <memory>

#undef EXIT_SUCCESS

// --- Constants ---
namespace Constants
{
const std::wstring REMOTE_INFO_URL =
	L"https://drive.google.com/uc?export=download&id=1dGYvJyYMzwWILVc8HOeORhIXeMRitbr3";
const std::wstring CURL_EXE = L"curl.exe";
const std::wstring SEVEN_ZIP_EXE = L"7za.exe";
const std::wstring LAUNCHER_EXE = L"Launcher.exe";
const std::wstring LOCAL_VERSION_FILE = L"appdata/network/version.txt";
const std::wstring TEMP_INFO_FILE = L"appdata/network/update_info_temp.txt";
const std::wstring TEMP_ARCHIVE = L"appdata/network/update_package.zip";
} // namespace Constants

enum ExitCodes
{
	EXIT_SUCCESS_CODE = 0,
	EXIT_UPDATE_AVAILABLE = 1,
	EXIT_NO_UPDATE = 2,
	EXIT_ERROR_NETWORK = -1,
	EXIT_ERROR_DOWNLOAD = -2,
	EXIT_ERROR_PARSING = -3,
	EXIT_ERROR_ARGS = -4,
	EXIT_ERROR_FILE_NOT_FOUND = -5,
	EXIT_ERROR_EXTRACTION = -6
};

// --- Utilities ---
class StringUtils
{
  public:
	static std::wstring ToWString(const std::string& str)
	{
		if (str.empty())
			return L"";
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	static std::string ToString(const std::wstring& wstr)
	{
		if (wstr.empty())
			return "";
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::string Trim(const std::string& str)
	{
		std::string s = str;
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
		return s;
	}
};

// --- Logger ---
class Logger
{
  public:
	static void Init(bool debugMode)
	{
		s_debugMode = debugMode;
		if (s_debugMode)
		{
			if (AllocConsole())
			{
				FILE* fp;
				freopen_s(&fp, "CONOUT$", "w", stdout);
				freopen_s(&fp, "CONOUT$", "w", stderr);
				freopen_s(&fp, "CONIN$", "r", stdin);
				SetConsoleOutputCP(CP_UTF8);
				SetConsoleCP(CP_UTF8);

				HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD dwMode = 0;
				GetConsoleMode(hOut, &dwMode);
				dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				SetConsoleMode(hOut, dwMode);

				std::ios::sync_with_stdio(false);
			}
		}
	}

	static void Info(const std::string& msg)
	{
		if (s_debugMode)
			std::cout << "[INFO] " << msg << std::endl;
	}
	static void Error(const std::string& msg)
	{
		if (s_debugMode)
			std::cerr << "\x1b[31m[ERROR] " << msg << "\x1b[0m" << std::endl;
	}
	static void Warning(const std::string& msg)
	{
		if (s_debugMode)
			std::cout << "\x1b[33m[WARNING] " << msg << "\x1b[0m" << std::endl;
	}
	static void Success(const std::string& msg)
	{
		if (s_debugMode)
			std::cout << "\x1b[32m[SUCCESS] " << msg << "\x1b[0m" << std::endl;
	}
	static void Debug(const std::string& msg)
	{
		if (s_debugMode)
			std::cout << "\x1b[36m[DEBUG] " << msg << "\x1b[0m" << std::endl;
	}
	static bool IsDebug()
	{
		return s_debugMode;
	}

  private:
	static bool s_debugMode;
};
bool Logger::s_debugMode = false;

// --- File System Helper ---
class FileSystem
{
  public:
	static std::wstring GetExeDir()
	{
		wchar_t buffer[MAX_PATH];
		GetModuleFileNameW(NULL, buffer, MAX_PATH);
		std::wstring path(buffer);
		return path.substr(0, path.find_last_of(L"\\/"));
	}

	static std::wstring JoinPath(std::wstring part1, std::wstring part2)
	{
		if (part1.empty())
			return part2;
		if (part2.empty())
			return part1;
		if (part1.back() != L'\\' && part1.back() != L'/')
			part1 += L'\\';
		return part1 + part2;
	}

	static bool Exists(const std::wstring& filepath)
	{
		DWORD fileAttr = GetFileAttributesW(filepath.c_str());
		return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
	}

	static bool CreateDirs(const std::wstring& workDir)
	{
		std::wstring appdataDir = JoinPath(workDir, L"appdata");
		std::wstring networkDir = JoinPath(appdataDir, L"network");

		if (!CreateDirectoryW(appdataDir.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			return false;
		if (!CreateDirectoryW(networkDir.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			return false;
		return true;
	}

	static std::string ReadFile(const std::wstring& path)
	{
		std::ifstream f(path);
		if (!f.is_open())
			return "";
		std::stringstream buffer;
		buffer << f.rdbuf();
		return buffer.str();
	}

	static bool WriteFile(const std::wstring& path, const std::string& content)
	{
		std::ofstream f(path);
		if (!f.is_open())
			return false;
		f << content;
		return true;
	}

	static void DeleteFileW(const std::wstring& path)
	{
		_wremove(path.c_str());
	}

	static long long GetFileSize(const std::wstring& path)
	{
		WIN32_FILE_ATTRIBUTE_DATA fileAttr;
		if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileAttr))
		{
			LARGE_INTEGER size;
			size.HighPart = fileAttr.nFileSizeHigh;
			size.LowPart = fileAttr.nFileSizeLow;
			return size.QuadPart;
		}
		return 0;
	}
};

// --- Process Runner ---
class ProcessRunner
{
  public:
	static int Run(const std::wstring& cmdLine, const std::wstring& workDir, bool showWindow)
	{
		Logger::Debug("Running: " + StringUtils::ToString(cmdLine));

		STARTUPINFOW si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		if (!showWindow)
		{
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
		}

		std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
		cmdBuffer.push_back(0);
		std::vector<wchar_t> workDirBuffer(workDir.begin(), workDir.end());
		workDirBuffer.push_back(0);

		if (!CreateProcessW(NULL, &cmdBuffer[0], NULL, NULL, TRUE, 0, NULL, workDir.empty() ? NULL : &workDirBuffer[0],
							&si, &pi))
		{
			Logger::Error("CreateProcess failed: " + std::to_string(GetLastError()));
			return -1;
		}

		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode = 0;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return (int)exitCode;
	}
};

// --- Config Parser ---
class ConfigParser
{
  public:
	static std::string GetValue(const std::string& content, const std::string& key)
	{
		std::istringstream stream(content);
		std::string line;
		while (std::getline(stream, line))
		{
			size_t eqPos = line.find('=');
			if (eqPos != std::string::npos)
			{
				std::string currentKey = line.substr(0, eqPos);
				// Simple trim
				currentKey.erase(std::remove_if(currentKey.begin(), currentKey.end(), isspace), currentKey.end());

				if (currentKey == key)
				{
					return StringUtils::Trim(line.substr(eqPos + 1));
				}
			}
		}
		return "";
	}
};

// --- Update Manager ---
class UpdateManager
{
  private:
	std::wstring m_workDir;
	std::wstring m_exeDir;

  public:
	UpdateManager(const std::wstring& workDir) : m_workDir(workDir)
	{
		m_exeDir = FileSystem::GetExeDir();
		FileSystem::CreateDirs(m_workDir);
	}

	bool DownloadFile(const std::string& url, const std::wstring& destPath, bool verbose)
	{
		std::wstring curlPath = FileSystem::JoinPath(m_exeDir, Constants::CURL_EXE);
		if (!FileSystem::Exists(curlPath))
		{
			Logger::Error("curl.exe not found at: " + StringUtils::ToString(curlPath));
			return false;
		}

		std::wstring wUrl = StringUtils::ToWString(url);
		std::wstring args = verbose ? L" -L -v -o \"" : L" -L -s -o \"";
		if (url != StringUtils::ToString(Constants::REMOTE_INFO_URL) && !verbose)
		{
			args = L" -L --progress-bar -o \""; // Show progress bar for large files if not in verbose
		}

		std::wstring cmd = L"\"" + curlPath + L"\"" + args + destPath + L"\" \"" + wUrl + L"\"";
		return ProcessRunner::Run(cmd, m_exeDir, Logger::IsDebug()) == 0;
	}

	bool ExtractArchive(const std::wstring& archivePath)
	{
		std::wstring sevenZipPath = FileSystem::JoinPath(m_exeDir, Constants::SEVEN_ZIP_EXE);

		// Strategy 1: 7-Zip
		if (FileSystem::Exists(sevenZipPath))
		{
			Logger::Info("Extracting with 7-Zip...");
			std::wstring cmd = L"\"" + sevenZipPath + L"\" x -aoa -o\"" + m_workDir + L"\" \"" + archivePath + L"\"";
			if (ProcessRunner::Run(cmd, m_exeDir, Logger::IsDebug()) == 0)
				return true;
		}
		else
		{
			Logger::Warning("7za.exe not found. Trying fallback...");
		}

		// Strategy 2: PowerShell Fallback
		Logger::Info("Attempting extraction with PowerShell...");
		std::wstring psCmd = L"powershell -command \"Expand-Archive -Path '" + archivePath + L"' -DestinationPath '" +
							 m_workDir + L"' -Force\"";
		return ProcessRunner::Run(psCmd, m_workDir, Logger::IsDebug()) == 0;
	}

	int Check()
	{
		std::wstring infoPath = FileSystem::JoinPath(m_workDir, Constants::TEMP_INFO_FILE);

		if (!DownloadFile(StringUtils::ToString(Constants::REMOTE_INFO_URL), infoPath, Logger::IsDebug()))
		{
			return EXIT_ERROR_NETWORK;
		}

		if (FileSystem::GetFileSize(infoPath) == 0)
		{
			Logger::Error("Downloaded info file is empty.");
			return EXIT_ERROR_NETWORK;
		}

		std::wstring localPath = FileSystem::JoinPath(m_workDir, Constants::LOCAL_VERSION_FILE);
		if (!FileSystem::Exists(localPath))
		{
			Logger::Warning("Local version file not found. Creating default.");
			FileSystem::WriteFile(localPath, "version=0.0.0\n");
		}

		std::string localVer = ConfigParser::GetValue(FileSystem::ReadFile(localPath), "version");
		std::string remoteVer = ConfigParser::GetValue(FileSystem::ReadFile(infoPath), "version");

		Logger::Debug("Local: " + localVer + ", Remote: " + remoteVer);

		if (remoteVer.empty())
			return EXIT_ERROR_PARSING;
		if (localVer != remoteVer)
		{
			Logger::Info("Update available: " + localVer + " -> " + remoteVer);
			return EXIT_UPDATE_AVAILABLE;
		}

		Logger::Info("No update available.");
		FileSystem::DeleteFileW(infoPath); // Cleanup
		return EXIT_NO_UPDATE;
	}

	int Update()
	{
		// 1. Download Info
		std::wstring infoPath = FileSystem::JoinPath(m_workDir, Constants::TEMP_INFO_FILE);
		if (!DownloadFile(StringUtils::ToString(Constants::REMOTE_INFO_URL), infoPath, Logger::IsDebug()))
		{
			return EXIT_ERROR_NETWORK;
		}

		std::string infoContent = FileSystem::ReadFile(infoPath);
		std::string downloadUrl = ConfigParser::GetValue(infoContent, "url");
		std::string newVersion = ConfigParser::GetValue(infoContent, "version");

		if (downloadUrl.empty())
		{
			Logger::Error("Download URL missing in info file.");
			return EXIT_ERROR_PARSING;
		}

		// 2. Download Archive
		std::wstring zipPath = FileSystem::JoinPath(m_workDir, Constants::TEMP_ARCHIVE);
		Logger::Info("Downloading update package...");
		if (!DownloadFile(downloadUrl, zipPath, false))
		{ // false for verbose = clean progress bar
			return EXIT_ERROR_DOWNLOAD;
		}

		// 3. Extract
		if (!ExtractArchive(zipPath))
		{
			Logger::Error("Extraction failed.");
			return EXIT_ERROR_EXTRACTION;
		}

		// 4. Update Version File
		std::wstring localVerPath = FileSystem::JoinPath(m_workDir, Constants::LOCAL_VERSION_FILE);
		if (!FileSystem::WriteFile(localVerPath, "version=" + newVersion + "\n"))
		{
			Logger::Error("Failed to write version file.");
		}

		// 5. Cleanup
		FileSystem::DeleteFileW(infoPath);
		FileSystem::DeleteFileW(zipPath);

		// 6. Restart Launcher
		std::wstring launcherPath = FileSystem::JoinPath(m_workDir, Constants::LAUNCHER_EXE);
		Logger::Success("Update complete. Restarting Launcher...");

		ShellExecuteW(NULL, L"open", launcherPath.c_str(), NULL, m_workDir.c_str(), SW_SHOW);

		return EXIT_SUCCESS_CODE;
	}
};

// --- Main ---
int wmain(int argc, wchar_t* argv[])
{
	std::wstring mode = L"";
	std::wstring workDir = FileSystem::GetExeDir();
	bool debug = false;

	// Parse Arguments
	for (int i = 1; i < argc; i++)
	{
		std::wstring arg = argv[i];
		if (arg == L"--check")
			mode = L"--check";
		else if (arg == L"--update")
			mode = L"--update";
		else if (arg == L"--debug" || arg == L"-v")
			debug = true;
		else if (arg == L"--work-dir" && i + 1 < argc)
		{
			workDir = argv[++i];
		}
	}

	// Auto-detect workdir (parent of parent) if not specified
	// Logic: bin/network/Updater.exe -> root/
	if (workDir == FileSystem::GetExeDir())
	{
		std::wstring parent = workDir;
		for (int k = 0; k < 2; ++k)
		{
			size_t pos = parent.find_last_of(L"\\/");
			if (pos != std::wstring::npos)
				parent = parent.substr(0, pos);
		}
		if (!parent.empty() && parent != workDir)
			workDir = parent;
	}

	Logger::Init(debug);

	if (debug)
	{
		Logger::Info("Debug Mode Enabled");
		Logger::Info("Work Dir: " + StringUtils::ToString(workDir));
	}

	UpdateManager manager(workDir);

	if (mode == L"--check")
	{
		return manager.Check();
	}
	else if (mode == L"--update")
	{
		Logger::Info("Closing launcher if running...");
		Sleep(2000); // Give time for launcher to close

		int result = manager.Update();

		return result;
	}
	else
	{
		if (debug)
		{
			Logger::Error("No mode specified. Use --check or --update");
			std::cin.get();
		}
		else
		{
			// Minimal help for non-debug
			std::cout << "Usage: UpdateService.exe --check | --update [--debug] [--work-dir path]\n";
		}
		return EXIT_ERROR_ARGS;
	}
}
