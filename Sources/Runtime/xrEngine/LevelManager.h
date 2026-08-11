#pragma once

// Структура информации об уровне
struct sLevelInfo
{
	char* folder;
	char* name;
};

class ENGINE_API CLevelManager
{
  private:
	xr_vector<sLevelInfo> Levels;
	u32 Level_Current;

	void Level_Append(LPCSTR folder);

  public:
	CLevelManager();
	~CLevelManager();

	void Scan(); // Бывший Level_Scan

	// Методы для получения данных (чтобы UI мог их читать)
	const xr_vector<sLevelInfo>& GetLevels() const
	{
		return Levels;
	}
	const sLevelInfo* GetCurrentLevelInfo() const;
	LPCSTR GetCurrentLevelFolderName() const;

	// Основная логика
	int GetLevelID(LPCSTR name);
	void SetLevel(u32 ID); // Тут будет только установка FS путей
};
