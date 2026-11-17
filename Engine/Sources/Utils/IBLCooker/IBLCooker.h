////////////////////////////////////////////////////////////////////////////////
// Created: 17.11.2026 19:44:56
// Author: NS_Deathman
// File: IBLCooker.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
////////////////////////////////////////////////////////////////////////////////
static const UINT TARGET_MIPS = 9;
static const UINT IRRADIANCE_SIZE = 512;
////////////////////////////////////////////////////////////////////////////////
class IBLCooker
{
public:
    IBLCooker();
    ~IBLCooker();

    // Один раз: device, шейдеры, состояния, cbuffer
    bool Init();

    // Один файл: <src>.dds  ->  <src>_ibl.dds + <src>_irradiance.dds
    bool ProcessFile(const std::wstring& sourcePath);

    // Все .dds в каталоге (пропускает *_ibl.dds и *_irradiance.dds)
    bool ProcessDirectory(const std::wstring& dirPath);

    void Shutdown();

private:
    bool InitDevice();
    bool InitShaders();
    bool InitPipelineStates();

    bool LoadSourceCubemap(const std::wstring& path);
    bool SaveTextureToFile(ID3D11Texture2D* texture, const std::wstring& path);

    bool BakeCombined(const std::wstring& outPath);   // color + spec в одном
    bool BakeIrradiance(const std::wstring& outPath); // отдельный 32x32

    ComPtr<ID3D11Texture2D> CreateCubemapTexture(UINT size, UINT mipLevels,
        DXGI_FORMAT format, UINT bindFlags);
    ComPtr<ID3D11RenderTargetView> CreateCubemapRTV(ID3D11Texture2D* texture,
        UINT mipSlice, UINT arraySlice);
    void UpdateConstantBuffer(float roughness, UINT faceIndex);
    void RenderFullscreenTriangle();

    ComPtr<ID3D11Device>            m_device;
    ComPtr<ID3D11DeviceContext>     m_context;
    ComPtr<ID3D11VertexShader>      m_fullscreenVS;
    ComPtr<ID3D11PixelShader>       m_irradiancePS;
    ComPtr<ID3D11PixelShader>       m_specularPS;
    ComPtr<ID3D11RasterizerState>   m_rasterizerState;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    ComPtr<ID3D11BlendState>        m_blendState;
    ComPtr<ID3D11SamplerState>      m_linearSampler;
    ComPtr<ID3D11Buffer>            m_constantBuffer;
    ComPtr<ID3D11Texture2D>         m_sourceCubemap;
    ComPtr<ID3D11ShaderResourceView> m_sourceCubemapSRV;

    UINT m_sourceSize = 0;

    struct BakeConstants
    {
        float roughness;   // 4
        UINT  faceIndex;   // 4
        float sourceSize;  // 4
        float _padding;    // 4  -> 16 байт, кратно 16
    };
};
////////////////////////////////////////////////////////////////////////////////
