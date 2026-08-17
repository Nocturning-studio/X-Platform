#include "stdafx.h"
#include "LevelManager.h"

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
		LI.name = 0; // “ут можно добавить чтение имени из level.ltx если нужно
		Levels.push_back(LI);
	}
}

void CLevelManager::Scan()
{
	Msg("Scanning levels...");

	// ќчищаем старое, если вызываем повторно
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
	// √лавна€ задача менеджера уровней Ч настроить файловую систему
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
