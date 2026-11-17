////////////////////////////////////////////////////////////////////////////////
// Created: 17.11.2026 19:45:07
// Author: NS_Deathman
// File: IBLCooker.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <d3dcompiler.h>
#include <filesystem>
#include <wrl/client.h>
#include "DirectXTex/DirectXTex.h"
#include "log.h"
////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
////////////////////////////////////////////////////////////////////////////////
namespace fs = std::filesystem;
////////////////////////////////////////////////////////////////////////////////
#include "IBLCooker.h"
////////////////////////////////////////////////////////////////////////////////
#define HR_CHECK(hr) if (FAILED(hr)) { return false; }

static UINT ComputeMipLevels(UINT size)
{
    UINT levels = 1;
    while (size > 1) { size >>= 1; ++levels; }
    return levels;
}

static fs::path GetShaderPath(const wchar_t* filename)
{
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return fs::path(exePath).parent_path() / L"shaders" / filename;
}

static ComPtr<ID3DBlob> CompileShader(const fs::path& file, const char* entry, const char* target)
{
    ComPtr<ID3DBlob> blob, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
    flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
#endif
    HRESULT hr = D3DCompileFromFile(file.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, target, flags, 0, &blob, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            Log(L"[IBLCooker] Shader compile error:\n");
            Log((const wchar_t*)error->GetBufferPointer());
        }
        return nullptr;
    }
    return blob;
}

IBLCooker::IBLCooker() {}
IBLCooker::~IBLCooker() { Shutdown(); }

bool IBLCooker::Init()
{
    if (!InitDevice())         return false;
    if (!InitShaders())        return false;
    if (!InitPipelineStates()) return false;
    return true;
}

bool IBLCooker::InitDevice()
{
    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
        nullptr, 0, D3D11_SDK_VERSION, &m_device, &featureLevel, &m_context);
    HR_CHECK(hr);
    return true;
}

bool IBLCooker::InitShaders()
{
    auto vsBlob = CompileShader(GetShaderPath(L"FullscreenTriangle.hlsl"), "main", "vs_5_0");
    if (!vsBlob) return false;
    HR_CHECK(m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, &m_fullscreenVS));

    auto irrBlob = CompileShader(GetShaderPath(L"IrradianceConvolution.hlsl"), "main", "ps_5_0");
    if (!irrBlob) return false;
    HR_CHECK(m_device->CreatePixelShader(irrBlob->GetBufferPointer(), irrBlob->GetBufferSize(),
        nullptr, &m_irradiancePS));

    auto specBlob = CompileShader(GetShaderPath(L"SpecularPrefilter.hlsl"), "main", "ps_5_0");
    if (!specBlob) return false;
    HR_CHECK(m_device->CreatePixelShader(specBlob->GetBufferPointer(), specBlob->GetBufferSize(),
        nullptr, &m_specularPS));
    return true;
}

bool IBLCooker::InitPipelineStates()
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    HR_CHECK(m_device->CreateRasterizerState(&rsDesc, &m_rasterizerState));

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;
    HR_CHECK(m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilState));

    D3D11_BLEND_DESC bDesc = {};
    bDesc.RenderTarget[0].BlendEnable = FALSE;
    bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    HR_CHECK(m_device->CreateBlendState(&bDesc, &m_blendState));

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR_CHECK(m_device->CreateSamplerState(&sampDesc, &m_linearSampler));

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(BakeConstants); // 16
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR_CHECK(m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer));
    return true;
}

ComPtr<ID3D11Texture2D> IBLCooker::CreateCubemapTexture(UINT size, UINT mipLevels,
    DXGI_FORMAT format, UINT bindFlags)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = size;
    desc.Height = size;
    desc.MipLevels = mipLevels;
    desc.ArraySize = 6;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = bindFlags;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &texture))) return nullptr;
    return texture;
}

ComPtr<ID3D11RenderTargetView> IBLCooker::CreateCubemapRTV(ID3D11Texture2D* texture, UINT mipSlice, UINT arraySlice)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texture->GetDesc(&texDesc);

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Texture2DArray.MipSlice = mipSlice;
    rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
    rtvDesc.Texture2DArray.ArraySize = 1;

    ComPtr<ID3D11RenderTargetView> rtv;
    if (FAILED(m_device->CreateRenderTargetView(texture, &rtvDesc, &rtv))) return nullptr;
    return rtv;
}

void IBLCooker::UpdateConstantBuffer(float roughness, UINT faceIndex)
{
    BakeConstants cb = {};
    cb.roughness = roughness;
    cb.faceIndex = faceIndex;
    cb.sourceSize = (float)m_sourceSize;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_context->Unmap(m_constantBuffer.Get(), 0);
    }
}

void IBLCooker::RenderFullscreenTriangle()
{
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(nullptr);
    m_context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    m_context->Draw(3, 0);
}

bool IBLCooker::LoadSourceCubemap(const std::wstring& path)
{
    m_sourceCubemap.Reset();
    m_sourceCubemapSRV.Reset();
    m_sourceSize = 0;

    Log(L"[IBLCooker] LoadSourceCubemap: %s", path.c_str());

    DirectX::TexMetadata metadata;
    DirectX::ScratchImage image;

    HRESULT hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
    {
        Log(L"[IBLCooker]   LoadFromDDSFile FAILED hr=0x%08X", (unsigned)hr);
        return false;
    }
    Log(L"[IBLCooker]   Loaded: %ux%u mips=%u arraySize=%u miscFlags=0x%X fmt=%u",
        (unsigned)metadata.width, (unsigned)metadata.height,
        (unsigned)metadata.mipLevels, (unsigned)metadata.arraySize,
        (unsigned)metadata.miscFlags, (unsigned)metadata.format);

    if (!metadata.IsCubemap()) { Log(L"[IBLCooker]   NOT a cubemap"); return false; }
    if (metadata.width != metadata.height) { Log(L"[IBLCooker]   Not square"); return false; }
    if (metadata.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
    {
        Log(L"[IBLCooker]   Too big: %u > 16384", (unsigned)metadata.width);
        return false;
    }

    // === ФИКС: приводим к рабочему формату ДО генерации мипов ===
    // BC-форматы нельзя генерировать мипы с TEX_FILTER_DEFAULT (ERROR_NOT_SUPPORTED).
    // R16G16B16A16_FLOAT — наш целевой формат, конвертируем сразу и генерируем мипы в float.
    if (metadata.format != DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
        Log(L"[IBLCooker]   Converting source fmt=%u -> R16G16B16A16_FLOAT", (unsigned)metadata.format);

        DirectX::ScratchImage decompressed;
        DirectX::ScratchImage converted;

        // Шаг 1: Декомпрессия BC1 (или другого BC-формата) в промежуточный R8G8B8A8_UNORM
        HRESULT hr = DirectX::Decompress(
            image.GetImages(), image.GetImageCount(), metadata,
            DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
        if (FAILED(hr))
        {
            Log(L"[IBLCooker]   Decompress FAILED hr=0x%08X", (unsigned)hr);
            return false;
        }
        Log(L"[IBLCooker]   Decompressed to R8G8B8A8_UNORM");

        // Шаг 2: Конвертация R8G8B8A8_UNORM в целевой R16G16B16A16_FLOAT
        hr = DirectX::Convert(
            decompressed.GetImages(), decompressed.GetImageCount(), decompressed.GetMetadata(),
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DirectX::TEX_FILTER_DEFAULT, 0, converted);
        if (FAILED(hr))
        {
            Log(L"[IBLCooker]   Convert FAILED hr=0x%08X", (unsigned)hr);
            return false;
        }
        Log(L"[IBLCooker]   Converted to R16G16B16A16_FLOAT");

        image = std::move(converted);
        metadata = image.GetMetadata();
    }

    // === Генерация мипов — теперь в float, без ограничений BC ===
    const UINT expectedMips = ComputeMipLevels((UINT)metadata.width);
    if (metadata.mipLevels < expectedMips)
    {
        Log(L"[IBLCooker]   Generating mips (%u -> %u)",
            (unsigned)metadata.mipLevels, expectedMips);

        DirectX::ScratchImage mipChain;
        hr = DirectX::GenerateMipMaps(
            image.GetImages(), image.GetImageCount(), metadata,
            DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
        if (FAILED(hr))
        {
            Log(L"[IBLCooker]   GenerateMipMaps FAILED hr=0x%08X", (unsigned)hr);
            return false;
        }
        image = std::move(mipChain);
        metadata = image.GetMetadata();
        Log(L"[IBLCooker]   Mips generated: %u levels", (unsigned)metadata.mipLevels);
    }

    ComPtr<ID3D11Resource> resource;
    hr = DirectX::CreateTexture(m_device.Get(), image.GetImages(), image.GetImageCount(),
        metadata, &resource);
    if (FAILED(hr))
    {
        Log(L"[IBLCooker]   CreateTexture FAILED hr=0x%08X", (unsigned)hr);
        return false;
    }

    hr = resource.As(&m_sourceCubemap);
    if (FAILED(hr)) { Log(L"[IBLCooker]   As<Texture2D> FAILED hr=0x%08X", (unsigned)hr); return false; }

    hr = m_device->CreateShaderResourceView(m_sourceCubemap.Get(), nullptr, &m_sourceCubemapSRV);
    if (FAILED(hr)) { Log(L"[IBLCooker]   CreateSRV FAILED hr=0x%08X", (unsigned)hr); return false; }

    m_sourceSize = (UINT)metadata.width;
    Log(L"[IBLCooker]   OK, sourceSize=%u", m_sourceSize);
    return true;
}

bool IBLCooker::SaveTextureToFile(ID3D11Texture2D* texture, const std::wstring& path)
{
    DirectX::ScratchImage scratch;
    HRESULT hr = DirectX::CaptureTexture(m_device.Get(), m_context.Get(), texture, scratch);
    if (FAILED(hr))
    {
        Log(L"[IBLCooker] CaptureTexture failed.\n");
        return false;
    }
    hr = DirectX::SaveToDDSFile(scratch.GetImages(), scratch.GetImageCount(),
        scratch.GetMetadata(), DirectX::DDS_FLAGS_NONE, path.c_str());
    if (FAILED(hr))
    {
        Log(L"[IBLCooker] SaveToDDSFile failed.\n");
        return false;
    }
    return true;
}

bool IBLCooker::BakeCombined(const std::wstring& outPath)
{
    const UINT size = m_sourceSize;
    const UINT mips = ComputeMipLevels(size);
    if (size == 0 || mips == 0) return false;

    // Диагностика на больших источниках (файл = 6 * Σ (size²/4^i) * 8 байт)
    if (size >= 1024)
    {
        Log(L"[IBLCooker] Large source: output DDS may be several hundred MB.\n");
    }

    auto combined = CreateCubemapTexture(size, mips, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (!combined) return false;

    m_context->RSSetState(m_rasterizerState.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    m_context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);

    m_context->PSSetShader(m_specularPS.Get(), nullptr, 0);
    m_context->PSSetSamplers(0, 1, m_linearSampler.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, m_sourceCubemapSRV.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    for (UINT mip = 0; mip < mips; ++mip)
    {
        // mip 0 -> roughness 0 -> бит-в-бит источник; последний mip -> roughness 1
        const float roughness = (mips > 1)
            ? (float)mip / (float)(mips - 1)
            : 0.0f;

        const UINT mipSize = (std::max)(1u, size >> mip);
        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)mipSize, (float)mipSize, 0.0f, 1.0f };
        m_context->RSSetViewports(1, &vp);

        for (UINT face = 0; face < 6; ++face)
        {
            UpdateConstantBuffer(roughness, face);
            auto rtv = CreateCubemapRTV(combined.Get(), mip, face);
            if (!rtv) continue;
            m_context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
            RenderFullscreenTriangle();
        }
    }

    return SaveTextureToFile(combined.Get(), outPath);
}

bool IBLCooker::BakeIrradiance(const std::wstring& outPath)
{
    auto irrTex = CreateCubemapTexture(IRRADIANCE_SIZE, 1,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (!irrTex) return false;

    m_context->RSSetState(m_rasterizerState.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    m_context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);

    m_context->PSSetShader(m_irradiancePS.Get(), nullptr, 0);
    m_context->PSSetSamplers(0, 1, m_linearSampler.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, m_sourceCubemapSRV.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)IRRADIANCE_SIZE, (float)IRRADIANCE_SIZE, 0.0f, 1.0f };
    m_context->RSSetViewports(1, &vp);

    for (UINT face = 0; face < 6; ++face)
    {
        UpdateConstantBuffer(0.0f, face);
        auto rtv = CreateCubemapRTV(irrTex.Get(), 0, face);
        if (!rtv) continue;
        m_context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        RenderFullscreenTriangle();
    }
    return SaveTextureToFile(irrTex.Get(), outPath);
}

bool IBLCooker::ProcessFile(const std::wstring& sourcePath)
{
    Log(L"[IBLCooker] ProcessFile: %s", sourcePath.c_str());
    if (!LoadSourceCubemap(sourcePath)) { Log(L"[IBLCooker]   load failed"); return false; }

    fs::path src(sourcePath);
    fs::path combinedPath = src.parent_path() / (src.stem().wstring() + L".dds");
    fs::path irradiancePath = src.parent_path() / (src.stem().wstring() + L"#irradiance.dds");

    bool ok1 = BakeCombined(combinedPath.wstring());
    Log(L"[IBLCooker]   BakeCombined -> %s", ok1 ? L"OK" : L"FAIL");
    bool ok2 = BakeIrradiance(irradiancePath.wstring());
    Log(L"[IBLCooker]   BakeIrradiance -> %s", ok2 ? L"OK" : L"FAIL");
    return ok1 && ok2;
}

bool IBLCooker::ProcessDirectory(const std::wstring& dirPath)
{
    fs::path dir(dirPath);
    Log(L"[IBLCooker] ProcessDirectory: %s", dir.c_str());

    std::error_code ec;

    if (!fs::exists(dir, ec))
    {
        Log(L"[IBLCooker] Directory missing, creating: %s", dir.c_str());
        fs::create_directories(dir, ec);
        if (ec)
        {
            Log(L"[IBLCooker] create_directories failed: ec=%d (%hs)",
                ec.value(), ec.message().c_str());
            return false;
        }
        Log(L"[IBLCooker] Created empty cubemaps dir. Put .dds there and rerun.");
        return false;
    }

    if (!fs::is_directory(dir, ec))
    {
        Log(L"[IBLCooker] Not a directory: %s", dir.c_str());
        return false;
    }

    bool any = false, allOk = true;

    for (auto it = fs::directory_iterator(dir, ec);
        !ec && it != fs::directory_iterator(); it.increment(ec))
    {
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext != L".dds") continue;

        const std::wstring stem = entry.path().stem().wstring();
        if (stem.size() >= 4 && stem.substr(stem.size() - 4) == L"")        continue;
        if (stem.size() >= 11 && stem.substr(stem.size() - 11) == L"#irradiance") continue;

        Log(L"[IBLCooker] Found input: %s", entry.path().c_str());
        any = true;
        if (!ProcessFile(entry.path().wstring()))
        {
            Log(L"[IBLCooker] FAILED: %s", entry.path().c_str());
            allOk = false;
        }
        else
        {
            Log(L"[IBLCooker] OK: %s", entry.path().c_str());
        }
    }

    if (ec)
    {
        Log(L"[IBLCooker] Directory iteration error: ec=%d (%hs)",
            ec.value(), ec.message().c_str());
        return false;
    }

    if (!any)
    {
        Log(L"[IBLCooker] No usable .dds in: %s", dir.c_str());
        return false;
    }

    Log(L"[IBLCooker] ProcessDirectory done, allOk=%d", (int)allOk);
    return allOk;
}

void IBLCooker::Shutdown()
{
    m_sourceCubemapSRV.Reset();
    m_sourceCubemap.Reset();
    m_constantBuffer.Reset();
    m_linearSampler.Reset();
    m_blendState.Reset();
    m_depthStencilState.Reset();
    m_rasterizerState.Reset();
    m_specularPS.Reset();
    m_irradiancePS.Reset();
    m_fullscreenVS.Reset();
    m_context.Reset();
    m_device.Reset();
}
////////////////////////////////////////////////////////////////////////////////
