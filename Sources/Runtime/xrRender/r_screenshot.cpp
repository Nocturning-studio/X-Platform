#include "stdafx.h"
#include "tga.h"
#include "..\xrEngine\xrImage_Resampler.h"
#include "..\xrEngine\XR_IOConsole.h"
#include <xrCore/build_identificator.h>

namespace
{
    std::atomic<bool> g_screenshotInProcess{ false };

    struct ScreenshotGuard
    {
        bool acquired = false;

        ScreenshotGuard()
        {
            bool expected = false;
            acquired = g_screenshotInProcess.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
        }

        ScreenshotGuard(ScreenshotGuard&& other) noexcept : acquired(other.acquired)
        {
            other.acquired = false;
        }

        ScreenshotGuard(const ScreenshotGuard&) = delete;
        ScreenshotGuard& operator=(const ScreenshotGuard&) = delete;
        ScreenshotGuard& operator=(ScreenshotGuard&&) = delete;

        ~ScreenshotGuard()
        {
            if (acquired)
                g_screenshotInProcess.store(false, std::memory_order_release);
        }
    };
}

void CRender::Screenshot(IRender_interface::ScreenshotMode mode, LPCSTR name)
{
    if (!Device.b_is_Ready)
        return;

    ScreenshotGuard guard;
    if (!guard.acquired)
    {
        Msg("* Screenshot: previous one is still being saved, skipping");
        return;
    }

    // Захват — обязательно на рендер-потоке (device call).
    IDirect3DSurface9* shot = RenderBackend.CaptureBackBuffer();
    if (!shot)
    {
        Msg("! Screenshot: failed to capture backbuffer");
        return;
    }

    string64  t_stemp;
    string_path file_name;

    switch (mode)
    {
        case IRender_interface::ScreenshotMode::SM_FOR_GAMESAVE:
        {
            std::string save_path = name ? name : "";
            static constexpr u32 GAMESAVE_THUMB_SIZE = 128;

            D3DSURFACE_DESC shotDesc;
            shot->GetDesc(&shotDesc);

            IDirect3DSurface9* thumb = nullptr;
            HRESULT hr = RenderBackend.GetDevice()->CreateOffscreenPlainSurface(GAMESAVE_THUMB_SIZE, 
                                                                                GAMESAVE_THUMB_SIZE, 
                                                                                shotDesc.Format, 
                                                                                D3DPOOL_SYSTEMMEM, 
                                                                                &thumb, 
                                                                                nullptr);

            if (FAILED(hr) || !thumb)
            {
                Msg("! Screenshot[gamesave]: failed to create %ux%u thumbnail surface (0x%08x)",
                    GAMESAVE_THUMB_SIZE, GAMESAVE_THUMB_SIZE, hr);
                _RELEASE(shot);
                return;
            }

            RenderBackend.BlitSurface(thumb, shot);

            auto guardPtr = std::make_shared<ScreenshotGuard>(std::move(guard));
            Engine.ThreadManager.AddBackgroundTask([shot, thumb, guardPtr, save_path]() mutable
            {
                ID3DXBuffer* saved = nullptr;
                RenderBackend.SaveSurfaceToMemory(&saved, D3DXIFF_DDS, thumb);
                if (saved)
                {
                    IWriter* fs = FS.w_open(save_path.c_str());
                    if (fs)
                    {
                        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                        FS.w_close(fs);
                    }
                    else
                    {
                        Msg("! Screenshot: failed to open '%s'", save_path.c_str());
                    }
                    _RELEASE(saved);
                }
                _RELEASE(thumb);
                _RELEASE(shot);
            });
            return;
        }

        case IRender_interface::ScreenshotMode::SM_NORMAL:
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
            strconcat(sizeof(file_name), file_name, file_name, ".png");

            std::string fname = file_name;
            auto guardPtr = std::make_shared<ScreenshotGuard>(std::move(guard));
            Engine.ThreadManager.AddBackgroundTask([shot, guardPtr, fname]() mutable
            {
                ID3DXBuffer* saved = nullptr;
                RenderBackend.SaveSurfaceToMemory(&saved, D3DXIFF_PNG, shot);
                if (saved)
                {
                    IWriter* fs = FS.w_open("$screenshots$", fname.c_str());
                    if (fs)
                    {
                        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                        FS.w_close(fs);
                    }
                    else
                    {
                        Msg("! Screenshot: failed to open '$screenshots$/%s'", fname.c_str());
                    }
                    _RELEASE(saved);
                }
                _RELEASE(shot);
            });
            return;
        }

        case IRender_interface::ScreenshotMode::SM_FOR_LEVELMAP:
        {
            if (!g_pGameLevel)
            {
                Msg("! Can't capture level map, level does no loaded");
                _RELEASE(shot);
                return;
            }

            sprintf_s(file_name, sizeof(string_path), "level_map_%s_%s.dds",
                g_pGameLevel->name().c_str(), timestamp(t_stemp));

            // D3D-ресурсы создаём на рендер-потоке. Всё остальное — в воркере.
            IDirect3DTexture9* texture = nullptr;
            RenderBackend.CreateTexture(2048, 2048, 1, NULL, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture, nullptr);
            IDirect3DSurface9* surface = RenderBackend.GetSurfaceLevel(texture, 0);

            std::string fname = file_name;

            auto guardPtr = std::make_shared<ScreenshotGuard>(std::move(guard));
            Engine.ThreadManager.AddBackgroundTask([shot, texture, surface, fname, guardPtr]() mutable
            {
                // DXT1-компрессия + запись DDS — это и есть основная работа.
                RenderBackend.BlitSurface(surface, shot);

                ID3DXBuffer* saved = nullptr;
                RenderBackend.SaveSurfaceToMemory(&saved, D3DXIFF_DDS, surface);
                if (saved)
                {
                    IWriter* fs = FS.w_open("$screenshots$", fname.c_str());
                    if (fs)
                    {
                        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                        FS.w_close(fs);
                    }
                    else
                    {
                        Msg("! Screenshot: failed to open '$screenshots$/%s'", fname.c_str());
                    }
                    _RELEASE(saved);
                }
                _RELEASE(surface);
                _RELEASE(texture);
                _RELEASE(shot);
            });
            return;
        }

        case IRender_interface::ScreenshotMode::SM_FOR_CUBEMAP:
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
                if (fs)
                {
                    fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                    FS.w_close(fs);
                }
                _RELEASE(saved);

                for (int i = 0; i < 6; ++i)
                    _RELEASE(surface[i]);
                _RELEASE(cubemap);
            }

            _RELEASE(shot);
            return;
        }
    }

    _RELEASE(shot);
}
