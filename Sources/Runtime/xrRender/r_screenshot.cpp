#include "stdafx.h"
#include "tga.h"
#include "..\xrEngine\xrImage_Resampler.h"
#include "..\xrEngine\XR_IOConsole.h"
#include <xrCore/build_identificator.h>

void CRender::Screenshot(IRender_interface::ScreenshotMode mode, LPCSTR name)
{
    if (!Device.b_is_Ready)
        return;

    IDirect3DSurface9* shot = RenderBackend.CaptureBackBuffer();
    if (!shot)
    {
        Msg("! Screenshot: failed to capture backbuffer");
        return;
    }

    D3DSURFACE_DESC sd;
    shot->GetDesc(&sd);
    RenderBackend.MakeOpaque(shot, sd.Width, sd.Height);

    string64  t_stemp;
    string_path file_name;

    switch (mode)
    {
    case IRender_interface::SM_FOR_GAMESAVE:
    {
        RenderBackend.BlitSurface(RenderTarget->surf_screenshot_gamesave, shot);

        ID3DXBuffer* saved = nullptr;
        RenderBackend.SaveTextureToMemory(&saved, D3DXIFF_DDS, RenderTarget->tex_screenshot_gamesave);

        IWriter* fs = FS.w_open(name);
        if (fs)
        {
            fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
            FS.w_close(fs);
        }
        _RELEASE(saved);
        return;
    }

    case IRender_interface::SM_NORMAL:
    {
        if (!name)
        {
#ifdef BENCHMARK_BUILD
            sprintf_s(file_name, sizeof(file_name),
                "X-Ray Benchmark (time - %s) (%s)",
                timestamp(t_stemp), g_pGameLevel->name().c_str());
#else
            sprintf_s(file_name, sizeof(file_name),
                "X-Ray Engine (build id - %d) (user - %s) (time - %s) (%s)",
                GlobalBuildInfo.ID, Core.UserName, timestamp(t_stemp),
                (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");
#endif
        }
        else
        {
            strcpy(file_name, name);
        }

        // Оставляем PNG как в исходнике (UsePngFormat всегда true).
        strconcat(sizeof(file_name), file_name, file_name, ".png");

        ID3DXBuffer* saved = nullptr;
        RenderBackend.SaveSurfaceToMemory(&saved, D3DXIFF_PNG, shot);

        IWriter* fs = FS.w_open("$screenshots$", file_name);
        R_ASSERT(fs);
        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
        FS.w_close(fs);
        _RELEASE(saved);
        return;
    }

    case IRender_interface::SM_FOR_LEVELMAP:
    {
        if (!g_pGameLevel)
        {
            Msg("! Can't capture level map, level does no loaded");
            return;
        }

        sprintf_s(file_name, sizeof(string_path), "level_map_%s_%s.dds", g_pGameLevel->name().c_str(), timestamp(t_stemp));

        IDirect3DTexture9* texture = nullptr;
        RenderBackend.CreateTexture(2048, 2048, 1, NULL, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture, nullptr);

        IDirect3DSurface9* surface = RenderBackend.GetSurfaceLevel(texture, 0);
        RenderBackend.BlitSurface(surface, shot);

        ID3DXBuffer* saved = nullptr;
        RenderBackend.SaveSurfaceToMemory(&saved, D3DXIFF_DDS, surface);

        IWriter* fs = FS.w_open("$screenshots$", file_name);
        R_ASSERT(fs);
        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
        FS.w_close(fs);

        _RELEASE(surface);
        _RELEASE(texture);
        _RELEASE(saved);
        return;
    }

    case IRender_interface::SM_FOR_CUBEMAP:
    {
        u32 face_size = ps_r_cubemap_size / 4;

        static IDirect3DCubeTexture9* cubemap = nullptr;
        static IDirect3DSurface9* surface[6] = { nullptr };

        u32 id = (int)name[0] - (int)'1';

        if (id == 0)
            RenderBackend.CreateCubeTexture(face_size, 1, NULL, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &cubemap, nullptr);

        D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)id;
        surface[id] = RenderBackend.GetCubeMapSurface(cubemap, face, 0);
        RenderBackend.BlitSurface(surface[id], shot);

        if (id == 5)
        {
            sprintf_s(file_name, sizeof(string_path), "cubemap_%s_%s.dds", Core.UserName, timestamp(t_stemp));

            ID3DXBuffer* saved = nullptr;
            RenderBackend.SaveTextureToMemory(&saved, D3DXIFF_DDS, cubemap);

            IWriter* fs = FS.w_open("$cubemaps$", file_name);
            R_ASSERT(fs);
            fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
            FS.w_close(fs);

            _RELEASE(saved);

            for (int i = 0; i < 6; ++i)
                _RELEASE(surface[i]);
            _RELEASE(cubemap);
        }
        return;
    }
    }
}
