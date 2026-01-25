// TextureManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)

#include "ResourceManager.h"
#include "tss.h"
#include "blender.h"
#include "blender_recorder.h"

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find(N);
#ifdef _EDITOR
	if (I == m_blenders.end())
		return 0;
#else
	if (I == m_blenders.end())
	{
		Debug.fatal(DEBUG_INFO, "Shader '%s' not found in library.", Name);
		return 0;
	}
#endif
	else
		return I->second;
}

IBlender* CResourceManager::_FindBlender(LPCSTR Name)
{
	if (!(Name && Name[0]))
		return 0;

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find(N);
	if (I == m_blenders.end())
		return 0;
	else
		return I->second;
}

void CResourceManager::ED_UpdateBlender(LPCSTR Name, IBlender* data)
{
	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find(N);
	if (I != m_blenders.end())
	{
		R_ASSERT(data->getDescription().CLS == I->second->getDescription().CLS);
		xr_delete(I->second);
		I->second = data;
	}
	else
	{
		m_blenders.insert(mk_pair(xr_strdup(Name), data));
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CResourceManager::_ParseList(sh_list& dest, LPCSTR names)
{
	if (0 == names)
		names = "$null";

	ZeroMemory(&dest, sizeof(dest));
	char* P = (char*)names;
	svector<char, 128> N;

	while (*P)
	{
		if (*P == ',')
		{
			// flush
			N.push_back(0);
			xr_strlwr(N.begin());

			Engine.ResourceManager->fix_texture_name(N.begin());
			dest.push_back(N.begin());
			N.clear();
		}
		else
		{
			N.push_back(*P);
		}
		P++;
	}
	if (N.size())
	{
		// flush
		N.push_back(0);
		xr_strlwr(N.begin());

		Engine.ResourceManager->fix_texture_name(N.begin());
		dest.push_back(N.begin());
	}
}

ShaderElement* CResourceManager::_CreateElement(ShaderElement& S)
{
	if (S.passes.empty())
		return 0;

	// Search equal in shaders array
	for (u32 it = 0; it < v_elements.size(); it++)
		if (S.equal(*(v_elements[it])))
			return v_elements[it];

	// Create _new_ entry
	ShaderElement* N = xr_new<ShaderElement>(S);
	N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_elements.push_back(N);
	return N;
}

void CResourceManager::_DeleteElement(const ShaderElement* S)
{
	if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_elements, S))
		return;
	Msg("! ERROR: Failed to find compiled 'shader-element'");
}

Shader* CResourceManager::_cpp_Create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants,
									  LPCSTR s_matrices)
{
	CBlender_Compile C;
	Shader S;

	// Access to template
	C.BT = B;
	C.bEditor = FALSE;
	C.bDetail = FALSE;
#ifdef _EDITOR
	if (!C.BT)
	{
		ELog.Msg(mtError, "Can't find shader '%s'", s_shader);
		return 0;
	}
	C.bEditor = TRUE;
#endif

	// Parse names
	_ParseList(C.L_textures, s_textures);
	_ParseList(C.L_constants, s_constants);
	_ParseList(C.L_matrices, s_matrices);

	// Compile element	(LOD0 - HQ)
	{
		C.iElement = 0;
		C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[0] = _CreateElement(E);
	}

	// Compile element	(LOD1)
	{
		C.iElement = 1;
		C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[1] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 2;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[2] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 3;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[3] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 4;
		C.bDetail = TRUE; //.$$$ HACK :)
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[4] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 5;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		S.E[5] = _CreateElement(E);
	}

	// Search equal in shaders array
	for (u32 it = 0; it < v_shaders.size(); it++)
		if (S.equal(v_shaders[it]))
			return v_shaders[it];

	// Create _new_ entry
	Shader* N = xr_new<Shader>(S);
	N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_shaders.push_back(N);
	return N;
}

Shader* CResourceManager::_cpp_Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
#ifndef DEDICATED_SERVER
	return _cpp_Create(_GetBlender(s_shader ? s_shader : "null"), s_shader, s_textures, s_constants, s_matrices);
#else
	return NULL;
#endif
}

Shader* CResourceManager::Create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
#ifndef DEDICATED_SERVER
	return _cpp_Create(B, s_shader, s_textures, s_constants, s_matrices);
#else
	return NULL;
#endif
}

Shader* CResourceManager::Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
#ifndef DEDICATED_SERVER
#ifndef _EDITOR
	if (_lua_HasShader(s_shader))
		return _lua_Create(s_shader, s_textures);
	else
#endif
		return _cpp_Create(s_shader, s_textures, s_constants, s_matrices);
#else
	return NULL;
#endif
}

void CResourceManager::Delete(const Shader* S)
{
	if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_shaders, S))
		return;
	Msg("! ERROR: Failed to find complete shader");
}

void CResourceManager::DeferredUpload()
{
	if (!Device.b_is_Ready)
		return;

	// 1. Собираем список текстур для загрузки
	xr_vector<CTexture*> textures_to_load;
	textures_to_load.reserve(m_textures.size());

	for (auto& pair : m_textures)
	{
		if (pair.second && !pair.second->flags.bLoaded)
			textures_to_load.push_back(pair.second);
	}

	// 2. Грузим пачками по N штук
	size_t total = textures_to_load.size();

	for (size_t i = 0; i < total; i++)
	{
		textures_to_load[i]->Load();
	}
}

void CResourceManager::DeferredUnload()
{
	if (!Device.b_is_Ready)
		return;

	Msg("* Texture unloading, size = %d", m_textures.size());

	CTimer timer;
	timer.Start();

	concurrency::parallel_for_each(m_textures.begin(), m_textures.end(), [](auto& iterator){iterator.second->Unload();});

	Msg("* Phase time: %d ms", timer.GetElapsed_ms());
}

void CResourceManager::DeferredUnloadLevelTextures(LPCSTR level_name)
{
	if (!Device.b_is_Ready)
		return;

	Msg("Unload level textures: %s", level_name);

	concurrency::parallel_for_each(m_textures.begin(), m_textures.end(), [](auto& iterator) {
		LPCSTR name = iterator.second->cName.c_str();

		LPCSTR templates[] = 
		{
			"build_details",
			"terrain\\",
			"level_lods",
			"lmap#",
		};

		for (int i = 0; i < sizeof(templates) / sizeof(LPCSTR); i++)
		{
			const char* first = strstr(name, templates[i]);
			const char* test = name;

			if (first == test)
			{
				iterator.second->Unload();
				break;
			}
		}
	});
}

#ifdef _EDITOR
void CResourceManager::ED_UpdateTextures(AStringVec* names)
{
	// 1. Unload
	if (names)
	{
		for (u32 nid = 0; nid < names->size(); nid++)
		{
			map_TextureIt I = m_textures.find((*names)[nid].c_str());
			if (I != m_textures.end())
				I->second->Unload();
		}
	}
	else
	{
		for (map_TextureIt t = m_textures.begin(); t != m_textures.end(); t++)
			t->second->Unload();
	}

	// 2. Load
	// DeferredUpload	();
}
#endif

void CResourceManager::_GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	m_base = c_base = m_lmaps = c_lmaps = 0;

	map_Texture::iterator I = m_textures.begin();
	map_Texture::iterator E = m_textures.end();
	for (; I != E; I++)
	{
		u32 m = I->second->flags.MemoryUsage;
		if (strstr(I->first, "lmap"))
		{
			c_lmaps++;
			m_lmaps += m;
		}
		else
		{
			c_base++;
			m_base += m;
		}
	}
}

void CResourceManager::_DumpMemoryUsage()
{
	xr_multimap<u32, std::pair<u32, shared_str>> mtex;

	// sort
	{
		map_Texture::iterator I = m_textures.begin();
		map_Texture::iterator E = m_textures.end();
		for (; I != E; I++)
		{
			u32 m = I->second->flags.MemoryUsage;
			shared_str n = I->second->cName;
			mtex.insert(mk_pair(m, mk_pair(I->second->dwReference, n)));
		}
	}

	// dump
	{
		xr_multimap<u32, std::pair<u32, shared_str>>::iterator I = mtex.begin();
		xr_multimap<u32, std::pair<u32, shared_str>>::iterator E = mtex.end();
		for (; I != E; I++)
			Msg("* %4.1f : [%4d] %s", float(I->first) / 1024.f, I->second.first, I->second.second.c_str());
	}
}

void CResourceManager::fix_texture_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if (_ext && (
		0 == xr_stricmp(_ext, ".tga") || 
		0 == xr_stricmp(_ext, ".dds") || 
		0 == xr_stricmp(_ext, ".bmp") ||
		0 == xr_stricmp(_ext, ".ogm") || 
		0 == xr_stricmp(_ext, ".hdr")))
		*_ext = 0;
}

void CResourceManager::Evict()
{
	CHK_DX(HW.pDevice->EvictManagedResources());
}
