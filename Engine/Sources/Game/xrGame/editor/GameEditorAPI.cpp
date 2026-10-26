////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:28:29
// Author: NS_Deathman
// File: GameEditorAPI.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GameEditorAPI.h"
#include <xrFramework/GameObjectFactory.h>
#include <xrFramework/IRender.h>
#include <xrFramework/Fmesh.h>
#include <xrFramework/SkeletonCustom.h>
#include <xrGame/xrServer_Objects.h>
#include <xrGame/gamemtllib.h>
#include <xrGame/smart_cast.h>
////////////////////////////////////////////////////////////////////////////////
extern void init_game_globals();
extern void clean_game_globals();
extern void CCC_RegisterCommands();
extern void CCC_RegisterInput();
////////////////////////////////////////////////////////////////////////////////
namespace
{
    CSE_Abstract* Editor_CreateServerObject(LPCSTR section)
    {
        return F_entity_Create(section);
    }

    void Editor_DestroyServerObject(CSE_Abstract* e)
    {
        if (e) F_entity_Destroy(e);
    }

    void Editor_RegisterModel(IRender_Visual* V)
    {
        // Тот же код, что в CGameContext::RegisterModel
        switch (V->Type)
        {
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID:
        {
            u16 def_idx = GMLib.GetMaterialIdx("default_object");
            CKinematics* K = smart_cast<CKinematics*>(V);
            VERIFY(K);
            int cnt = K->LL_BoneCount();
            for (u16 k = 0; k < cnt; k++)
            {
                CBoneData& bd = K->LL_GetData(k);
                if (*(bd.game_mtl_name))
                    bd.game_mtl_idx = GMLib.GetMaterialIdx(*bd.game_mtl_name);
                else
                    bd.game_mtl_idx = def_idx;
            }
        }
        break;
        }
    }

    float Editor_MtlTransparent(u32 mtl_idx)
    {
        return GMLib.GetMaterialByIdx((u16)mtl_idx)->fVisTransparencyFactor;
    }

    const SEditorGameAPI g_editor_api =
    {
        /*version*/           1,
        /*CreateServerObject*/Editor_CreateServerObject,
        /*DestroyServerObject*/Editor_DestroyServerObject,
        /*InitGameGlobals*/   &init_game_globals,
        /*CleanGameGlobals*/  &clean_game_globals,
        /*RegisterModel*/     Editor_RegisterModel,
        /*MtlTransparent*/    Editor_MtlTransparent,
        /*RegisterConsole*/   &CCC_RegisterCommands,
        /*RegisterInput*/     &CCC_RegisterInput,
    };
}

extern "C" DLL_API SEditorGameAPI * __cdecl xrGame_GetEditorAPI()
{
    return const_cast<SEditorGameAPI*>(&g_editor_api);
}
////////////////////////////////////////////////////////////////////////////////
