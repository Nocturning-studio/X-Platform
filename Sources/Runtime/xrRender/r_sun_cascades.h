#pragma once
#include "RenderScene.h"

struct ShadowCascadeWorkItem
{
    SSceneVisibilityResult vis_result;   // packet + context + matrices

    fmat4x4  cull_transform;
    fvec3    cull_COP;
    CFrustum cull_frustum;
    CSector* cull_sector;

    ShadowCascadeWorkItem() : cull_sector(nullptr) {}
};

struct SunCascadeBuffer
{
    ShadowCascadeWorkItem* items[3];

    SunCascadeBuffer()
    {
        for (int i = 0; i < 3; ++i)
            items[i] = nullptr;
    }

    void Init()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!items[i])
                items[i] = xr_new<ShadowCascadeWorkItem>();
            items[i]->vis_result.InitResources();
        }
    }

    void Destroy()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (items[i])
            {
                items[i]->vis_result.FreeResources();
                xr_delete(items[i]);
            }
        }
    }

    void Clear()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (items[i])
                items[i]->vis_result.Clear();
        }
    }
};

namespace Sun
{
    struct Ray
    {
        fvec3 Direction;
        fvec3 Position;

        Ray()
        {
        }
        Ray(fvec3 const& _P, fvec3 const& _D) : Position(_P), Direction(_D)
        {
        }
    };

    struct Cascade
    {
        Cascade() : reset_chain(false) {}

        xr_vector<Ray>	rays;
        float			size;
        bool			reset_chain;
    };

} //namespace sun
