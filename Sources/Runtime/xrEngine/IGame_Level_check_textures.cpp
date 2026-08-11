#include "stdafx.h"
#include "resourcemanager.h"
#include "igame_level.h"

void IGame_Level::LL_CheckTextures()
{
	u32 m_base, c_base, m_lmaps, c_lmaps;
	Engine.ResourceManager->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);

	Msg("* Summary memory usage for base textures: %d, %d K", c_base, m_base / 1024);
	Msg("* Summary memory usage for lmap textures: %d, %d K", c_lmaps, m_lmaps / 1024);
}
