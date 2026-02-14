#pragma once

#include "ShaderMacros.h"
#include <Windows.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <set>
#include <boost/crc.hpp>

// Вспомогательная функция для получения времени модификации файла
static time_t GetFileModTime(const char* filename)
{
	string_path full_path;
	FS.update_path(full_path, "$game_shaders$", filename);

	struct _stat fileStat;
	if (_stat(full_path, &fileStat) == 0)
	{
		return fileStat.st_mtime;
	}

	// Если не нашли по относительному пути, попробуем абсолютный
	if (_stat(filename, &fileStat) == 0)
	{
		return fileStat.st_mtime;
	}

	Msg("! Cannot get mod time for file: %s", filename);
	return 0;
}

// Вспомогательная функция для показа диалога с ошибкой
static void ShowShaderErrorDialog(const char* title, const std::string& message, bool isWarning = false)
{
	// В режиме MASTER_GOLD не показываем диалоги
#ifdef MASTER_GOLD
	return;
#else
	// Форматируем сообщение для диалога (убираем лишние символы форматирования)
	std::string dialogMessage = message;

	// Заменяем символы форматирования лога на более читаемые для диалога
	size_t pos = 0;
	while ((pos = dialogMessage.find("~", pos)) != std::string::npos)
	{
		dialogMessage.replace(pos, 1, "");
	}

	// Убираем лишние переносы в начале
	while (!dialogMessage.empty() && (dialogMessage[0] == '\n' || dialogMessage[0] == '!'))
	{
		dialogMessage.erase(0, 1);
	}

	// Обрезаем слишком длиные сообщения
	if (dialogMessage.length() > 2000)
	{
		dialogMessage = dialogMessage.substr(0, 2000) + "\n\n... (message truncated)";
	}

	UINT type = MB_OK | MB_ICONERROR | MB_TOPMOST;
	if (isWarning)
	{
		type = MB_OK | MB_ICONWARNING | MB_TOPMOST;
	}

	MessageBoxA(NULL, dialogMessage.c_str(), title, type);
#endif
}

// Структура для хранения метаданных кеша (должна быть POD и выровнена)
#pragma pack(push, 1)
struct ShaderCacheMetadata
{
	u32 crc;
	time_t sourceModTime;
	u32 buildId;
	u32 dependencyCount; // Количество зависимостей
};
#pragma pack(pop)

struct ShaderDependencyInfo
{
	std::string filePath;
	time_t modTime;
	u32 crc;
};

// Менеджер зависимостей шейдеров
class CShaderDependencyManager
{
  private:
	static const u32 DEPENDENCY_VERSION = 1;

  public:
	static bool WriteDependencyInfo(const std::string& cacheFilePath,
									const std::vector<ShaderDependencyInfo>& dependencies)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Starting WriteDependencyInfo");
#endif

		// БЕЗОПАСНОЕ формирование пути к INI файлу
		string_path iniPath;

		// Копируем базовый путь без расширения
		const char* xrcache_ext = strstr(cacheFilePath.c_str(), ".xrcache");
		if (xrcache_ext)
		{
			size_t base_len = xrcache_ext - cacheFilePath.c_str();
			if (base_len < sizeof(iniPath) - 5) // Оставляем место для ".xrdep" + null
			{
				strncpy_s(iniPath, cacheFilePath.c_str(), base_len);
				strcat_s(iniPath, ".xrdep");
			}
			else
			{
				Msg("! [DEPS] Path too long: %s", cacheFilePath.c_str());
				return false;
			}
		}
		else
		{
			Msg("! [DEPS] Invalid cache file path (no .xrcache extension): %s", cacheFilePath.c_str());
			return false;
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Writing dependency info to: %s", iniPath);
		Msg("* [DEPS] Number of dependencies to write: %d", dependencies.size());
#endif

		// Используем относительный путь для записи
		IWriter* file = FS.w_open("$app_data_root$", iniPath);
		if (!file)
		{
			Msg("! [DEPS] Cannot open dependency file for writing: %s", iniPath);
			return false;
		}

		// Записываем версию
		file->w_u32(DEPENDENCY_VERSION);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Written version: %d", DEPENDENCY_VERSION);
#endif

		// Записываем количество зависимостей
		u32 depCount = (u32)dependencies.size();
		file->w_u32(depCount);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Written dependency count: %d", depCount);
#endif

		// Записываем каждую зависимость
		for (u32 i = 0; i < depCount; i++)
		{
			const auto& dep = dependencies[i];

			// Длина пути + данные
			u16 pathLen = (u16)dep.filePath.length();
			file->w_u16(pathLen);
			file->w(dep.filePath.c_str(), pathLen);
			file->w_u64((u64)dep.modTime);
			file->w_u32(dep.crc);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Written dependency %d: %s (modTime: %lld, CRC: %u)", i, dep.filePath.c_str(),
				(long long)dep.modTime, dep.crc);
#endif
		}

		u32 fileSize = file->tell();
		FS.w_close(file);

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Successfully wrote dependency file: %s (%d bytes)", iniPath, fileSize);
#endif
		return true;
	}

	static bool ReadDependencyInfo(const std::string& cacheFilePath, std::vector<ShaderDependencyInfo>& dependencies)
	{
		string_path iniPath;
		strcpy_s(iniPath, cacheFilePath.c_str());

		// БЕЗОПАСНАЯ замена расширения
		char* ext = strstr(iniPath, ".xrcache");
		if (ext)
		{
			// Вычисляем сколько места осталось в буфере
			size_t remaining_size = sizeof(iniPath) - (ext - iniPath);
			if (remaining_size >= 6) // ".xrdep" + null terminator
			{
				strcpy_s(ext, remaining_size, ".xrdep");
			}
			else
			{
				Msg("! [DEPS] Not enough buffer space for extension replacement in ReadDependencyInfo");
				return false;
			}
		}
		else
		{
			// Если не нашли .xrcache, попробуем как есть (для обратной совместимости)
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] No .xrcache extension found, using path as-is: %s", iniPath);
#endif
		}

		// Используем относительный путь для чтения
		IReader* file = FS.r_open("$app_data_root$", iniPath);
		if (!file)
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Cannot open dependency file for reading: %s", iniPath);
#endif
			return false;
		}

		if (file->length() < sizeof(u32) + sizeof(u32))
		{
			Msg("! [DEPS] Dependency file too small: %s (%d bytes)", iniPath, file->length());
			FS.r_close(file);
			return false;
		}

		// Читаем версию
		u32 version = file->r_u32();
		if (version != DEPENDENCY_VERSION)
		{
			Msg("! [DEPS] Version mismatch in dependency file: %s (expected %d, got %d)", iniPath, DEPENDENCY_VERSION,
				version);
			FS.r_close(file);
			return false;
		}

		// Читаем количество зависимостей
		u32 depCount = file->r_u32();
		dependencies.clear();

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Reading %d dependencies from: %s", depCount, iniPath);
#endif

		for (u32 i = 0; i < depCount; i++)
		{
			if (file->elapsed() < sizeof(u16))
			{
				Msg("! [DEPS] Not enough data for path length at dependency %d", i);
				break;
			}

			ShaderDependencyInfo dep;
			u16 pathLen = file->r_u16();

			if (file->elapsed() < pathLen + sizeof(u64) + sizeof(u32))
			{
				Msg("! [DEPS] Not enough data for dependency %d (pathLen: %d, elapsed: %d)", i, pathLen,
					file->elapsed());
				break;
			}

			// Читаем путь
			char* pathBuf = (char*)alloca(pathLen + 1);
			file->r(pathBuf, pathLen);
			pathBuf[pathLen] = 0;
			dep.filePath = pathBuf;

			// Читаем время модификации и CRC
			dep.modTime = (time_t)file->r_u64();
			dep.crc = file->r_u32();

			dependencies.push_back(dep);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Read dependency %d: %s (modTime: %lld, CRC: %u)", i, dep.filePath.c_str(),
				(long long)dep.modTime, dep.crc);
#endif
		}

		FS.r_close(file);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Successfully read %d dependencies from: %s", dependencies.size(), iniPath);
#endif
		return true;
	}

	static bool AreDependenciesValid(const std::vector<ShaderDependencyInfo>& dependencies)
	{
		for (const auto& dep : dependencies)
		{
			string_path full_path;
			FS.update_path(full_path, "$game_shaders$", dep.filePath.c_str());

			time_t currentModTime = GetFileModTime(full_path);
			if (currentModTime == 0)
			{
				// Файл не найден - считаем зависимость невалидной
				Msg("! Dependency file not found: %s", dep.filePath.c_str());
				return false;
			}

			if (currentModTime != dep.modTime)
			{
				Msg("! Dependency changed: %s (was: %lld, now: %lld)", dep.filePath.c_str(), (long long)dep.modTime,
					(long long)currentModTime);
				return false;
			}

			// Дополнительная проверка CRC для важных файлов
			if (ShouldCheckCRC(dep.filePath))
			{
				u32 currentCRC = CalculateFileCRC(dep.filePath.c_str());
				if (currentCRC != dep.crc)
				{
					Msg("! Dependency CRC mismatch: %s (was: %u, now: %u)", dep.filePath.c_str(), dep.crc, currentCRC);
					return false;
				}
			}
		}
		return true;
	}

	static u32 CalculateFileCRC(const char* filePath)
	{
		string_path full_path;
		FS.update_path(full_path, "$game_shaders$", filePath);

		IReader* file = FS.r_open(full_path);
		if (!file)
		{
			// Попробуем открыть как относительный путь
			file = FS.r_open("$game_shaders$", filePath);
			if (!file)
			{
				Msg("! Cannot open file for CRC calculation: %s", filePath);
				return 0;
			}
		}

		boost::crc_32_type processor;
		processor.process_block(file->pointer(), ((char*)file->pointer()) + file->length());
		u32 crc = processor.checksum();

		FS.r_close(file);
		return crc;
	}

	static bool ShouldCheckCRC(const std::string& filePath)
	{
		// Проверяем CRC только для .h и .xrh файлов (шейдерные заголовки)
		const char* ext = strrchr(filePath.c_str(), '.');
		return ext && (strcmp(ext, ".h") == 0 || strcmp(ext, ".hlsl") == 0 || strcmp(ext, ".xrh") == 0 ||
					   strcmp(ext, ".xrps") == 0 || strcmp(ext, ".xrvs") == 0);
	}
};

//----------------------------------------------------------------
class CShaderIncluder : public ID3DXInclude
{
  private:
	u32 counter = 0;
	static const u32 max_size = 64;
	static const u32 max_guard_size = 1;
	char data[max_size * 1024];

	// Отслеживание зависимостей
	std::vector<std::string> includedFiles;
	std::string currentShaderName;

  public:
	void SetCurrentShader(const char* shaderName)
	{
		currentShaderName = shaderName;
		includedFiles.clear();
	}

	const std::vector<std::string>& GetIncludedFiles() const
	{
		return includedFiles;
	}

	HRESULT __stdcall Open(D3DXINCLUDE_TYPE type, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
	{
		bool shared = type == D3DXINC_SYSTEM;
		LPCSTR shaders_path = shared ? "shared\\" : ::Render->getShaderPath();

		static string_path full_path;
		sprintf_s(full_path, "%s%s", shaders_path, pName);

		// Добавляем файл в список зависимостей
		includedFiles.push_back(full_path);

		IReader* R = FS.r_open("$game_shaders$", full_path);

		if (R == NULL)
			return E_FAIL;

		if (R->length() + 1 + max_guard_size >= max_size * 1024)
		{
			Msg("! max shader file size: %uKB", max_size);
			return E_FAIL;
		}

		static string_path hash;
		size_t i = 0;
		for (i = 0; i < strlen(full_path); i++)
		{
			hash[i] = (full_path[i] >= '0' && full_path[i] <= '9') || (full_path[i] >= 'a' && full_path[i] <= 'z') ||
							  (full_path[i] >= 'A' && full_path[i] <= 'Z')
						  ? full_path[i]
						  : '_';
		}
		hash[i] = 0;

		char* offset = data;

		sprintf(offset, "#ifndef _%s_included\n", hash);
		offset = strchr(offset, 0);

		memcpy(offset, R->pointer(), R->length());
		offset += R->length();

		sprintf(offset, "\n#define _%s_included\n", hash);
		offset = strchr(offset, 0);

		sprintf(offset, "\n#endif");
		offset = strchr(offset, 0);

		FS.r_close(R);

		*ppData = data;
		*pBytes = (UINT)strlen(data);

#ifdef DEBUG_SHADER_COMPILATION
		//Msg("*   includer open: (id:%u): %s", counter, pName);
		Log(data);
		//Msg("*   guard                  _%s_included", hash);
#endif

		counter++;

		return D3D_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData)
	{
		return D3D_OK;
	}
};

//----------------------------------------------------------------
template <typename T> T* CResourceManager::RegisterShader(const char* _name)
{
	T* sh = xr_new<T>();
	sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	sh_map.insert(mk_pair(sh->set_name(_name), sh));
	return sh;
}

template <typename T> T* CResourceManager::FindShader(const char* _name)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(_name);

	if (I != sh_map.end())
		return I->second;

	// if the shader is not supported or not exist
	if (!ShaderTypeTraits<T>::IsSupported() ||
		(xr_strlen(_name) >= 4 && _name[0] == 'n' && _name[1] == 'u' && _name[2] == 'l' && _name[3] == 'l'))
	{
		T* sh = RegisterShader<T>("null");
		sh->sh = NULL;
		return sh;
	}

	return NULL;
}

#ifdef DEBUG_SHADER_COMPILATION
void print_macros(CShaderMacros& macros)
{
	if (macros.get_name().length() < 1)
		return;

	Msg("*   macro count: %d", macros.get_name().length());

	for (u32 i = 0; i < macros.get_impl().size(); ++i)
	{
		CShaderMacros::MacroImpl m = macros.get_impl()[i];
		Msg("*   macro: (%s : %s : %c)", m.Name, m.Definition, m.State);
	}
}
#endif

template <typename T> T* CResourceManager::CreateShader(const char* _name, const char* _entry, CShaderMacros& _macros)
{
	// get shader macros
	CShaderMacros macros;
	macros.add(::Render->FetchShaderMacros());
	macros.add(_macros);
	macros.add(TRUE, NULL, NULL);

	// Нормализуем входное имя функции
	LPCSTR actual_entry = (_entry && _entry[0]) ? _entry : "main";

	// make the unique shader name
	// [ИЗМЕНЕНО] Уникальное имя теперь включает имя функции (entry point),
	// чтобы различать test.ps(main) и test.ps(MainPS)
	string_path name;
	sprintf_s(name, sizeof name, "%s_%s%s", _name, actual_entry, macros.get_name().c_str());

	// if the shader is already exist, return it
	T* sh = FindShader<T>(name);

	if (sh)
		return sh;

	// create a new shader
	sh = RegisterShader<T>(name);

	// open file
	string_path file_source;
	const char* ext = ShaderTypeTraits<T>::GetShaderExt();
	sprintf_s(file_source, sizeof file_source, "%s%s.%s", ::Render->getShaderPath(), _name, ext);
	FS.update_path(file_source, "$game_shaders$", file_source);
	IReader* file = FS.r_open(file_source);

	if (!file)
	{
		std::string errorMsg =
			make_string("Cannot open shader file:\n%s\n\nShader will not be available.", file_source);
		Msg("! %s", errorMsg.c_str());

		// Показываем диалог об ошибке
		std::string dialogTitle = make_string("Shader File Error - %s.%s", _name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), errorMsg, false);

		// Возвращаем шейдер с NULL указателем вместо ассерта
		sh->sh = NULL;
		return sh;
	}

	// select target
	const char* type = ShaderTypeTraits<T>::GetShaderType(); // Возвращает "ps" или "vs"
	string32 c_target;
	sprintf_s(c_target, sizeof c_target, "%s_%u_%u", type, HW.Caps.raster_major, HW.Caps.raster_minor);

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* Compiling shader: target=%s, source=%s.%s, entry=%s", c_target, _name, ext, actual_entry);
	print_macros(_macros);
#endif

	// [НОВАЯ ЛОГИКА] Попытка компиляции с fallback
	// 1. Пробуем с запрошенным именем (обычно "main" или пользовательское)
	HRESULT _hr =
		CompileShader(_name, ext, (LPCSTR)file->pointer(), file->length(), c_target, actual_entry, macros, (T*&)sh);

	// 2. Если не вышло И мы пробовали стандартный "main", пробуем альтернативы MainPS/MainVS
	if (FAILED(_hr) && (0 == xr_strcmp(actual_entry, "main")))
	{
		string32 alt_entry;
		// Определяем альтернативное имя на основе типа шейдера (первая буква типа 'p' -> ps -> MainPS)
		if (type[0] == 'p')
			strcpy_s(alt_entry, "MainPS");
		else
			strcpy_s(alt_entry, "MainVS");

		Msg("! Compile failed for entry 'main', trying alternative: '%s' for shader %s", alt_entry, _name);

		_hr = CompileShader(_name, ext, (LPCSTR)file->pointer(), file->length(), c_target, alt_entry, macros, (T*&)sh);

		if (SUCCEEDED(_hr))
		{
			Msg("* Alternative entry point '%s' compiled successfully.", alt_entry);
		}
	}

	FS.r_close(file);

	// Если компиляция не удалась, все равно возвращаем шейдер (с NULL указателем)
	return sh;
}

template <typename T> HRESULT CResourceManager::ReadShaderCache(string_path name, T*& result, time_t sourceModTime)
{
	// Используем относительный путь для чтения
	IReader* file = FS.r_open("$app_data_root$", name);
	HRESULT cache_result = E_FAIL;

	if (file && file->length() > sizeof(ShaderCacheMetadata))
	{
		// Читаем метаданные
		ShaderCacheMetadata metadata;
		file->r(&metadata, sizeof(ShaderCacheMetadata));

#ifdef DEBUG_SHADER_COMPILATION
		// Отладочная информация
		Msg("* Cache check: buildId=%d (expected %d), modTime=%lld (expected %lld)", metadata.buildId, build_id,
			(long long)metadata.sourceModTime, (long long)sourceModTime);
#endif

		// Проверяем актуальность кеша
		if (metadata.buildId == build_id && metadata.sourceModTime == sourceModTime)
		{
			// ВРЕМЕННО: Отключаем систему зависимостей для отладки
			bool bEnableDependencySystem = false;
#ifndef MASTER_GOLD
			bEnableDependencySystem = true; // strstr(Core.Params, "-use_shader_dependencies") != nullptr;
#endif

			// Проверяем зависимости только если система включена
			std::vector<ShaderDependencyInfo> dependencies;
			if (bEnableDependencySystem && CShaderDependencyManager::ReadDependencyInfo(name, dependencies))
			{
				if (CShaderDependencyManager::AreDependenciesValid(dependencies))
				{
					// Вычисляем CRC оставшихся данных (сам шейдер)
					u32 data_size = file->elapsed();
					boost::crc_32_type processor;
					processor.process_block(((char*)file->pointer()), ((char*)file->pointer()) + data_size);
					u32 const real_crc = processor.checksum();

					if (real_crc == metadata.crc)
					{
#ifdef DEBUG_SHADER_COMPILATION
						Msg("* Cache CRC valid: %u, all dependencies valid", real_crc);
#endif
						cache_result = ReflectShader((DWORD*)(file->pointer()), data_size, result);
					}
					else
					{
						Msg("! Shader cache CRC mismatch for: %s (expected: %u, got: %u)", name, metadata.crc,
							real_crc);
					}
				}
				else
				{
					Msg("! Dependencies changed for: %s", name);
				}
			}
			else
			{
				// Пропускаем проверку зависимостей
#ifdef DEBUG_SHADER_COMPILATION
				Msg("* Skipping dependency check (system disabled or no info)");
#endif
				// Проверяем только CRC основного шейдера
				u32 data_size = file->elapsed();
				boost::crc_32_type processor;
				processor.process_block(((char*)file->pointer()), ((char*)file->pointer()) + data_size);
				u32 const real_crc = processor.checksum();

				if (real_crc == metadata.crc)
				{
#ifdef DEBUG_SHADER_COMPILATION
					Msg("* Cache CRC valid: %u", real_crc);
#endif
					cache_result = ReflectShader((DWORD*)(file->pointer()), data_size, result);
				}
				else
				{
					Msg("! Shader cache CRC mismatch for: %s (expected: %u, got: %u)", name, metadata.crc, real_crc);
				}
			}
		}
		else
		{
			if (metadata.buildId != build_id)
				Msg("! Shader cache build ID mismatch for: %s (expected: %d, got: %d)", name, build_id,
					metadata.buildId);
			else
				Msg("! Shader source file modified for: %s", name);
		}
	}
	else
	{
		if (!file)
			Msg("! Cannot open cache file: %s", name);
		else
			Msg("! Cache file too small: %s (%d bytes)", name, file->length());
	}

	if (file)
		FS.r_close(file);

	return cache_result;
}

template <typename T> HRESULT CResourceManager::ReflectShader(DWORD const* src, UINT size, T*& result)
{
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [REFLECT] Starting ReflectShader");
#endif

	result->sh = ShaderTypeTraits<T>::D3DCreateShader(src, size);

	if (result->sh == NULL)
	{
		Msg("! [REFLECT] Failed to create shader");
		return E_FAIL;
	}

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [REFLECT] Shader created successfully");
#endif

	const void* pReflection = 0;
	HRESULT _hr = D3DXFindShaderComment(src, MAKEFOURCC('C', 'T', 'A', 'B'), &pReflection, NULL);

	if (SUCCEEDED(_hr) && pReflection)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [REFLECT] Parsing constants...");
#endif
		result->constants.parse((void*)pReflection, ShaderTypeTraits<T>::GetShaderDest());
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [REFLECT] Constants parsed successfully");
#endif
		return _hr;
	}
	else
	{
		Msg("! [REFLECT] No reflection data found");
	}

	return E_FAIL;
}

static CTimer shader_compilation_timer;
extern XRCORE_API u32 build_id;

// Функция для создания директорий
static bool EnsureCacheDirectoryExists(const char* cachePath)
{
	string_path directory;
	strcpy_s(directory, cachePath);

	// Находим последний слэш
	char* lastSlash = strrchr(directory, '\\');
	if (!lastSlash)
		return true; // Нет директорий для создания

	*lastSlash = 0; // Обрезаем до директории

	// Проверяем существование и создаем если нужно
	//if (!FS.path_exist(directory))
	//{
		//Msg("* Creating cache directory: %s", directory);
		// if (!FS.create_path("$app_data_root$", directory))
		//{
		//	Msg("! Failed to create cache directory: %s", directory);
		//	return false;
		//}
	//}

	return true;
}

template <typename T>
HRESULT CResourceManager::CompileShader(LPCSTR name, LPCSTR ext, LPCSTR src, UINT size, LPCSTR target, LPCSTR entry,
										CShaderMacros& macros, T*& result)
{
	bool bUseShaderCache = true;
	bool bDisassebleShader = true;
	bool bWarningsAsErrors = false;
	bool bErrorsAsWarnings = false;

#ifndef MASTER_GOLD
	bUseShaderCache = !strstr(Core.Params, "-do_not_use_shader_cache");
	bDisassebleShader = strstr(Core.Params, "-save_disassembled_shaders");
	bWarningsAsErrors = strstr(Core.Params, "-shader_warnings_as_errors");
	bErrorsAsWarnings = strstr(Core.Params, "-shader_errors_as_warnings");

#ifdef DEBUG_SHADER_COMPILATION
	Msg("\n* [COMPILE] Starting shader compilation: %s.%s", name, ext);
	shader_compilation_timer.Start();
#endif
#endif

	HRESULT _result = E_FAIL;

	// ФОРМИРУЕМ ОТНОСИТЕЛЬНЫЙ путь к кешу (без FS.update_path)
	string_path cache_dest;
	sprintf_s(cache_dest, sizeof cache_dest, "shaders_cache\\%s%s.%s\\%s_%s_%d.xrcache", ::Render->getShaderPath(),
			  name, ext, macros.get_name().c_str(), entry, build_id);

	// Получаем время модификации исходного файла шейдера
	string_path source_file_path;
	sprintf_s(source_file_path, sizeof source_file_path, "%s%s.%s", ::Render->getShaderPath(), name, ext);
	FS.update_path(source_file_path, "$game_shaders$", source_file_path);
	time_t sourceModTime = GetFileModTime(source_file_path);

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Source file: %s, mod time: %lld", source_file_path, (long long)sourceModTime);
	Msg("* [COMPILE] Cache file: %s", cache_dest);
#endif

	// Try to find and read cache file - используем ОТНОСИТЕЛЬНЫЙ путь
	if (FS.exist("$app_data_root$", cache_dest) && bUseShaderCache)
	{
		_result = ReadShaderCache(cache_dest, result, sourceModTime);

		if (SUCCEEDED(_result))
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("- [COMPILE] Successfully found and applied compiled shader cache file");
#endif
			return _result;
		}
		else
		{
			Msg("! [COMPILE] Cache file invalid or outdated for shader: %s", cache_dest);
		}
	}

	CShaderIncluder Includer;
	ID3DXBuffer* pShaderBuf = NULL;
	ID3DXBuffer* pErrorBuf = NULL;
	ID3DXConstantTable* pConstants = NULL;

	// Устанавливаем текущий шейдер для includer'а
	Includer.SetCurrentShader(name);

	u32 flags = D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_PREFER_FLOW_CONTROL | D3DXSHADER_OPTIMIZATION_LEVEL3
#ifndef MASTER_GOLD
	//| D3DXSHADER_DEBUG
#endif
		;

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Calling D3DXCompileShader...");
#endif
	_result = D3DXCompileShader(src, size, &macros.get_macros()[0], &Includer, entry, target, flags, &pShaderBuf,
								&pErrorBuf, &pConstants);
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] D3DXCompileShader completed with result: 0x%08x", _result);
#endif

	if (SUCCEEDED(_result) && pShaderBuf)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Shader compilation successful, gathering dependencies...");
#endif

		// Собираем информацию о зависимостях (с удалением дубликатов)
		std::vector<ShaderDependencyInfo> dependencies;
		const auto& includedFiles = Includer.GetIncludedFiles();
		std::set<std::string> uniqueFiles; // Для проверки уникальности

		for (const auto& includedFile : includedFiles)
		{
			// Пропускаем дубликаты
			if (uniqueFiles.find(includedFile) != uniqueFiles.end())
				continue;

			uniqueFiles.insert(includedFile);

			ShaderDependencyInfo dep;
			dep.filePath = includedFile;

			// Получаем правильный путь для проверки
			string_path check_path;
			FS.update_path(check_path, "$game_shaders$", includedFile.c_str());

			dep.modTime = GetFileModTime(check_path);
			if (dep.modTime == 0)
			{
				// Если не нашли, пробуем исходный путь
				dep.modTime = GetFileModTime(includedFile.c_str());
			}

			// Если все еще 0, файл не найден - пропускаем или устанавливаем флаг ошибки
			if (dep.modTime == 0)
			{
				Msg("! [COMPILE] Warning: Cannot get mod time for dependency: %s", includedFile.c_str());
				// Продолжаем, но без этой зависимости
				continue;
			}

			dep.crc = CShaderDependencyManager::CalculateFileCRC(includedFile.c_str());
			dependencies.push_back(dep);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [COMPILE] Dependency: %s (modTime: %lld, CRC: %u)", includedFile.c_str(), (long long)dep.modTime,
				dep.crc);
#endif
		}

		if (bUseShaderCache)
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [COMPILE] Writing cache files...");
#endif

			// СОЗДАЕМ ДИРЕКТОРИИ перед записью файла
			if (!EnsureCacheDirectoryExists(cache_dest))
			{
				Msg("! [COMPILE] Failed to create cache directory for: %s", cache_dest);
			}
			else
			{
				// ИСПОЛЬЗУЕМ ОТНОСИТЕЛЬНЫЙ путь для записи!
				IWriter* file = FS.w_open("$app_data_root$", cache_dest);
				if (file)
				{
					// Вычисляем CRC данных шейдера
					boost::crc_32_type processor;
					processor.process_block(pShaderBuf->GetBufferPointer(),
											((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
					u32 crc = processor.checksum();

					// Подготавливаем метаданные с информацией о зависимостях
					ShaderCacheMetadata metadata;
					metadata.crc = crc;
					metadata.sourceModTime = sourceModTime;
					metadata.buildId = build_id;
					metadata.dependencyCount = (u32)dependencies.size();

#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Writing cache metadata with %d dependencies", dependencies.size());
#endif

					// Записываем метаданные и данные шейдера
					file->w(&metadata, sizeof(ShaderCacheMetadata));
					file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
					FS.w_close(file);

#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Main cache file written successfully");
#endif

					// Записываем информацию о зависимостях в отдельный INI файл
#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Writing dependency info...");
#endif
					if (CShaderDependencyManager::WriteDependencyInfo(cache_dest, dependencies))
					{
#ifdef DEBUG_SHADER_COMPILATION
						Msg("* [COMPILE] Dependency file written successfully");
#endif
					}
					else
					{
						Msg("! [COMPILE] Failed to write dependency file");
					}
				}
				else
				{
					Msg("! [COMPILE] Cannot write cache file: %s", cache_dest);
				}
			}
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Before ReflectShader");
#endif
		_result = ReflectShader((DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), result);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] After ReflectShader, result: 0x%08x", _result);
#endif

		if (FAILED(_result))
		{
			Msg("! [COMPILE] D3DReflectShader %s.%s hr == 0x%08x", name, ext, _result);
			// Не ассертим здесь - просто продолжаем с ошибкой
		}

		// Сохранение дизассемблированных данных шейдера
		if (bDisassebleShader && SUCCEEDED(_result))
		{
			ID3DXBuffer* pDisassembly = NULL;
			HRESULT disasmResult = D3DXDisassembleShader((DWORD*)pShaderBuf->GetBufferPointer(),
														 TRUE, // не показывать комментарии
														 NULL, // не использовать дополнительные комментарии
														 &pDisassembly);

			if (SUCCEEDED(disasmResult) && pDisassembly)
			{
				// Формируем путь для сохранения дизассемблированного шейдера
				string_path disasm_path;
				sprintf_s(disasm_path, sizeof disasm_path, "shaders_cache\\%s%s.%s\\%s_%s_%d.html",
						  ::Render->getShaderPath(), name, ext, macros.get_name().c_str(), entry, build_id);

				// Создаем директорию если нужно
				EnsureCacheDirectoryExists(disasm_path);

				// Сохраняем дизассемблированный код
				IWriter* disasm_file = FS.w_open("$app_data_root$", disasm_path);
				if (disasm_file)
				{
					disasm_file->w(pDisassembly->GetBufferPointer(), (u32)pDisassembly->GetBufferSize());
					FS.w_close(disasm_file);
					Msg("* [COMPILE] Disassembled shader saved to: %s", disasm_path);
				}
				else
				{
					Msg("! [COMPILE] Failed to save disassembled shader: %s", disasm_path);
				}

				_RELEASE(pDisassembly);
			}
			else
			{
				Msg("! [COMPILE] Failed to disassemble shader: 0x%08x", disasmResult);
			}
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Releasing shader buffer");
#endif
		_RELEASE(pShaderBuf);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Shader buffer released");
#endif
	}
	else
	{
		Msg("! [COMPILE] Shader compilation failed or no shader buffer");
	}

	// Обработка ошибок и предупреждений
	string16 code;
	sprintf_s(code, sizeof code, "%08x", _result);
	std::string message = pErrorBuf ? std::string((char*)pErrorBuf->GetBufferPointer()) : "";

	if (pErrorBuf && !FAILED(_result) && message.length() > 0)
	{
		std::string warning = make_string("\n~ Warning:\n~ Shader: %s\n~ Type: %s\n~ Target: %s\n~ HRESULT: %s\n", name,
										  ext, target, code);

		warning += message;

		Log(warning.c_str());
		FlushLog();
		
		// Вместо CHECK_OR_EXIT просто логируем предупреждение
		if (!bWarningsAsErrors)
		{
			Msg("! [COMPILE] Warnings treated as errors for shader: %s.%s", name, ext);
		}
		else
		{
			// Показываем диалог с предупреждением
			std::string dialogTitle = make_string("Shader Warning - %s.%s", name, ext);
			ShowShaderErrorDialog(dialogTitle.c_str(), warning, true);
		}
	}

	if (FAILED(_result))
	{
		std::string error = make_string("");

#ifdef MASTER_GOLD
		error = make_string("An error occurred during the shader compilation process.\nReport the problem to the "
							"developer, error:\nShader file: %s\nShader type: %s\nShader target: %s\nError: %s\n",
							name, ext, target, code);
#else
		error = make_string("\n! [COMPILE] Error:\n! Shader: %s\n! Type: %s\n! Target: %s\n! HRESULT: %s\n", name, ext,
							target, code);
#endif
		error += message;

		Log(error.c_str());
		FlushLog();

		// Показываем диалог с ошибкой
		std::string dialogTitle = make_string("Shader Compilation Error - %s.%s", name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), error, false);

		// Устанавливаем шейдер в NULL чтобы избежать использования битого шейдера
		if (result)
		{
			result->sh = NULL;
		}
	}
#ifndef MASTER_GOLD
	else
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("- [COMPILE] Shader successfully compiled");
		Msg("- [COMPILE] Compile time: %d ms", shader_compilation_timer.GetElapsed_ms());
#endif
	}
#endif

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Releasing DX buffers");
#endif
	if (pErrorBuf)
		_RELEASE(pErrorBuf);
	if (pConstants)
		_RELEASE(pConstants);
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] DX buffers released");
	Msg("* [COMPILE] CompileShader completed");
#endif

	return _result;
}

// Дополнительная утилита для поиска зависимостей
void CResourceManager::RecompileDependentShaders(const std::string& changedHeader)
{
	// Эта функция может быть вызвана при обнаружении изменения header-файла
	// Она найдет все шейдеры, которые зависят от этого header'а и пометит их кеш как устаревший

	string_path searchPattern;
	sprintf_s(searchPattern, "shaders_cache\\*\\*.xrcache.xrdep");

	FS_FileSet files;
	FS.file_list(files, "$app_data_root$", FS_ListFiles, searchPattern);

	int recompiledCount = 0;

	for (const auto& file : files)
	{
		// В XRay Engine FS_FileSet содержит только name, путь формируем относительно $app_data_root$
		string_path iniPath;
		sprintf_s(iniPath, "shaders_cache\\%s", file.name.c_str());

		std::vector<ShaderDependencyInfo> dependencies;
		if (CShaderDependencyManager::ReadDependencyInfo(iniPath, dependencies))
		{
			for (const auto& dep : dependencies)
			{
				if (dep.filePath.find(changedHeader) != std::string::npos)
				{
					// Нашли шейдер, зависящий от измененного header'а
					string_path cacheFile;
					strcpy_s(cacheFile, iniPath); // Используем strcpy_s вместо присваивания
					char* ext = strstr(cacheFile, ".xrdep");
					if (ext)
						*ext = 0; // Убираем .xrdep

					Msg("! Marking shader as outdated due to header change: %s", cacheFile);
					// Удаляем файлы кеша
					FS.file_delete("$app_data_root$", cacheFile);
					FS.file_delete("$app_data_root$", iniPath);
					recompiledCount++;
					break;
				}
			}
		}
	}

	if (recompiledCount > 0)
	{
		Msg("* Recompiled %d shaders due to change in: %s", recompiledCount, changedHeader.c_str());
	}
}

template <typename T> void CResourceManager::DestroyShader(const T* sh)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();

	if (0 == (sh->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	LPSTR N = LPSTR(*sh->cName);
	typename ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(N);

	if (I != sh_map.end())
	{
		sh_map.erase(I);
		return;
	}
}