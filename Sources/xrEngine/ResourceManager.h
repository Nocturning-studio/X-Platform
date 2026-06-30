// TextureManager.h: interface for the CTextureManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ResourceManagerH
#define ResourceManagerH
#pragma once

#include "shader.h"
#include "tss_def.h"
#include "TextureDescrManager.h"
#include "ShaderMacros.h"

// refs
struct lua_State;

// defs
class ENGINE_API CResourceManager
{
  private:
	struct str_pred
	{
		IC bool operator()(LPCSTR x, LPCSTR y) const
		{
			return xr_strcmp(x, y) < 0;
		}
	};

	struct texture_detail
	{
		const char* T;
		R_constant_setup* cs;
	};

  public:
	DEFINE_MAP_PRED(const char*, IBlender*, map_Blender, map_BlenderIt, str_pred);
	DEFINE_MAP_PRED(const char*, CTexture*, map_Texture, map_TextureIt, str_pred);
	DEFINE_MAP_PRED(const char*, CRT*, map_RT, map_RTIt, str_pred);
	DEFINE_MAP_PRED(const char*, CRTC*, map_RTC, map_RTCIt, str_pred);
	DEFINE_MAP_PRED(const char*, SVS*, map_VS, map_VSIt, str_pred);
	DEFINE_MAP_PRED(const char*, SPS*, map_PS, map_PSIt, str_pred);
	DEFINE_MAP_PRED(const char*, texture_detail, map_TD, map_TDIt, str_pred);

  private:
	// data
	map_Blender m_blenders;
	map_Texture m_textures;
	map_RT m_rtargets;
	map_RTC m_rtargets_c;
	map_VS m_vs;
	map_PS m_ps;
	map_TD m_td;

	xr_vector<SState*> v_states;
	xr_vector<SDeclaration*> v_declarations;
	xr_vector<SGeometry*> v_geoms;
	xr_vector<R_constant_table*> v_constant_tables;

	// lists
	xr_vector<STextureList*> lst_textures;

	// main shader-array
	xr_vector<SPass*> v_passes;
	xr_vector<ShaderElement*> v_elements;
	xr_vector<Shader*> v_shaders;

	xr_vector<ref_texture> m_necessary;
	// misc
  public:
	CTextureDescrMngr m_textures_description;
	xr_vector<std::pair<shared_str, R_constant_setup*>> v_constant_setup;
	lua_State* LSVM;
	BOOL bDeferredLoad;

  private:
	void LS_Load();
	void LS_Unload();

  public:
	// Miscelaneous
	void _ParseList(sh_list& dest, LPCSTR names);
	IBlender* _GetBlender(LPCSTR Name);
	IBlender* _FindBlender(LPCSTR Name);
	void _GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps);
	void _DumpMemoryUsage();
	void fix_texture_name(LPSTR fn);

	map_Blender& _GetBlenders()
	{
		return m_blenders;
	}

	CTexture* m_LoadedTexture;
	LPCSTR m_loadingTextureName;

	// Low level resource creation
	CTexture* _CreateTexture(LPCSTR _Name);
	void _DeleteTexture(const CTexture* T);

	R_constant_table* _CreateConstantTable(R_constant_table& C);
	void _DeleteConstantTable(const R_constant_table* C);

	CRT* _CreateRT(LPCSTR Name, u32 w, u32 h, xrRHI::RHI_Format f, u32 levels = 1);
	void _DeleteRT(const CRT* RT);

	CRTC* _CreateRTC(LPCSTR Name, u32 size, xrRHI::RHI_Format f, u32 levels = 1);
	void _DeleteRTC(const CRTC* RT);

	SPass* _CreatePass(ref_state& _state, ref_ps& _ps, ref_vs& _vs, ref_ctable& _ctable, ref_texture_list& _T);
	void _DeletePass(const SPass* P);

	// Shader compiling / optimizing
	SState* _CreateState(SimulatorStates& Code);
	void _DeleteState(const SState* SB);

	SDeclaration* _CreateDecl(D3DVERTEXELEMENT9* dcl);
	void _DeleteDecl(const SDeclaration* dcl);

	STextureList* _CreateTextureList(STextureList& L);
	void _DeleteTextureList(const STextureList* L);

	ShaderElement* _CreateElement(ShaderElement& L);
	void _DeleteElement(const ShaderElement* L);

	Shader* _cpp_Create(LPCSTR s_shader, LPCSTR s_textures = 0);
	Shader* _cpp_Create(IBlender* B, LPCSTR s_shader = 0, LPCSTR s_textures = 0);
	Shader* _lua_Create(LPCSTR s_shader, LPCSTR s_textures);
	BOOL _lua_HasShader(LPCSTR s_shader);

	CResourceManager() : bDeferredLoad(TRUE)
	{
	}
	~CResourceManager();

	void OnDeviceCreate(IReader* F);
	void OnDeviceCreate(LPCSTR name);
	void OnDeviceDestroy(BOOL bKeepTextures);

	void reset_begin();
	void reset_end();

	// Creation/Destroying
	Shader* Create(LPCSTR s_shader = 0, LPCSTR s_textures = 0);
	Shader* Create(IBlender* B, LPCSTR s_shader = 0, LPCSTR s_textures = 0);
	void Delete(const Shader* S);
	void RegisterConstantSetup(LPCSTR name, R_constant_setup* s)
	{
		v_constant_setup.push_back(mk_pair(shared_str(name), s));
	}

	SGeometry* CreateGeom(D3DVERTEXELEMENT9* decl, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib);
	SGeometry* CreateGeom(u32 FVF, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib);
	void DeleteGeom(const SGeometry* VS);
	void DeferredLoad(BOOL E)
	{
		bDeferredLoad = E;
	}

	void __stdcall ProcessUpload();
	void DeferredUpload();

	void DeferredUnload();
	void DeferredUnloadLevelTextures(LPCSTR level_name);
	void Evict();
	void StoreNecessaryTextures();
	void DestroyNecessaryTextures();
	void Dump(bool bBrief);

	template <typename T> T& GetShaderMap();
	template <typename T> T* FindShader(const char* _name);
	template <typename T> T* RegisterShader(const char* _name);
	template <typename T>
	HRESULT CompileShader(	LPCSTR name, 
							LPCSTR ext, 
							LPCSTR src, 
							UINT size, 
							LPCSTR target, 
							LPCSTR entry,
							CShaderMacros& macros, 
							T*& result	);

	// [ИЗМЕНЕНО] Добавлен аргумент const char* _entry = "main"
	template <typename T> T* CreateShader(const char* _name, const char* _entry, CShaderMacros& macros);

	// Для обратной совместимости можно добавить перегрузку (не обязательно, если везде обновили)
	template <typename T> T* CreateShader(const char* _name, CShaderMacros& macros)
	{
		return CreateShader<T>(_name, "main", macros);
	}
	template <typename T> void DestroyShader(const T* sh);
	template <typename T> HRESULT ReadShaderCache(string_path name, T*& result, time_t sourceModTime);
	template <typename T> HRESULT ReflectShader(DWORD const* src, UINT size, T*& result);
	void RecompileDependentShaders(const std::string& changedHeader);
};

template <class T> BOOL reclaim(xr_vector<T*>& vec, const T* ptr)
{
	xr_vector<T*>::iterator it = vec.begin();
	xr_vector<T*>::iterator end = vec.end();
	for (; it != end; it++)
		if (*it == ptr)
		{
			vec.erase(it);
			return TRUE;
		}
	return FALSE;
}

template SPS* CResourceManager::CreateShader<SPS>(LPCSTR _name, LPCSTR _entry, CShaderMacros& macros);
template SVS* CResourceManager::CreateShader<SVS>(LPCSTR _name, LPCSTR _entry, CShaderMacros& macros);

template void CResourceManager::DestroyShader<SPS>(const SPS* sh);
template void CResourceManager::DestroyShader<SVS>(const SVS* sh);

#endif // ResourceManagerH
