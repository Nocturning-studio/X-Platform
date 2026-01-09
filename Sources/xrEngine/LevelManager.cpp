#include "stdafx.h"
#include "LevelManager.h"
#include "Engine.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_ioconsole.h"
#include "std_classes.h"

CLevelManager::CLevelManager()
{
	Level_Current = 0;
}

CLevelManager::~CLevelManager()
{
	for (u32 i = 0; i < Levels.size(); i++)
	{
		xr_free(Levels[i].folder);
		xr_free(Levels[i].name);
	}
	Levels.clear();
}

void CLevelManager::Level_Append(LPCSTR folder)
{
	string_path N1, N2, N3, N4;
	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");

	if (FS.exist("$game_levels$", N1) && FS.exist("$game_levels$", N2) && FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4))
	{
		sLevelInfo LI;
		LI.folder = xr_strdup(folder);
		LI.name = 0; // Тут можно добавить чтение имени из level.ltx если нужно
		Levels.push_back(LI);
	}
}

void CLevelManager::Scan()
{
	// Очищаем старое, если вызываем повторно
	for (u32 i = 0; i < Levels.size(); i++)
	{
		xr_free(Levels[i].folder);
		xr_free(Levels[i].name);
	}
	Levels.clear();

	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
	R_ASSERT(folder && folder->size());

	for (u32 i = 0; i < folder->size(); i++)
		Level_Append((*folder)[i]);

	FS.file_list_close(folder);

#ifdef DEBUG
	folder = FS.file_list_open("$game_levels$", "$debug$\\", FS_ListFolders | FS_RootOnly);
	if (folder)
	{
		string_path tmp_path;
		for (u32 i = 0; i < folder->size(); i++)
		{
			strconcat(sizeof(tmp_path), tmp_path, "$debug$\\", (*folder)[i]);
			Level_Append(tmp_path);
		}
		FS.file_list_close(folder);
	}
#endif
}

void CLevelManager::StartGame(LPCSTR op_server, LPCSTR op_client)
{
	// 0. Валидация
	R_ASSERT(g_pGamePersistent);
	// Нельзя стартовать, если уровень уже есть (нужно сначала стопнуть)
	R_ASSERT2(0 == g_pGameLevel, "Level already exists! Call StopGame() first.");

	// 1. Подготовка UI
	Console->Execute("main_menu off");
	Console->Hide();

	// 2. PreStart (подготовка персистента)
	g_pGamePersistent->PreStart(op_server);

	// 3. Физическое создание класса уровня (из xrGame.dll)
	g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);

	// 4. Показываем загрузочный экран
	// Теперь мы сами управляем этим процессом, а не дергаем pApp
	Engine.LoadingScreen.Show();

	Msg("\n[CLevelManager]: Start level loading...");

	// 5. Запуск логики
	g_pGamePersistent->Start(op_server);		   // Старт сервера (или сингла)
	g_pGameLevel->net_Start(op_server, op_client); // Старт сетевой части и загрузка геометрии

	// 6. Скрываем загрузочный экран
	Engine.LoadingScreen.Hide();
}

void CLevelManager::StopGame()
{
	// Если уровня нет, то и останавливать нечего (кроме дисконнекта персистента)
	if (g_pGameLevel)
	{
		Msg("[CLevelManager]: Stopping level...");

		// Останавливаем сеть и логику уровня
		g_pGameLevel->net_Stop();

		// Уничтожаем объект уровня
		DEL_INSTANCE(g_pGameLevel);
	}

	// Отключаем персистент (сброс соединения)
	if (g_pGamePersistent)
		g_pGamePersistent->Disconnect();
}


int CLevelManager::GetLevelID(LPCSTR name)
{
	char buffer[256];
	strconcat(sizeof(buffer), buffer, name, "\\");
	for (u32 I = 0; I < Levels.size(); I++)
	{
		if (0 == xr_stricmp(buffer, Levels[I].folder))
			return int(I);
	}
	return -1;
}

void CLevelManager::SetLevel(u32 ID)
{
	if (ID >= Levels.size())
		return;

	Level_Current = ID;
	// Главная задача менеджера уровней — настроить файловую систему
	FS.get_path("$level$")->_set(Levels[ID].folder);
}

const sLevelInfo* CLevelManager::GetCurrentLevelInfo() const
{
	if (Levels.empty() || Level_Current >= Levels.size())
		return nullptr;
	return &Levels[Level_Current];
}

LPCSTR CLevelManager::GetCurrentLevelFolderName() const
{
	const sLevelInfo* info = GetCurrentLevelInfo();
	return info ? info->folder : nullptr;
}
