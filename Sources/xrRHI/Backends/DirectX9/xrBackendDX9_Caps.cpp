#include "pch.h"
#include "xrBackendDX9.h"

RHI_BEGIN

XRRHI_API std::string DecodeShaderVersion(u32 version)
{
    if (version == 0)
        return "None";

    unsigned int high = (version >> 16) & 0xFFFF;
    unsigned int low = version & 0xFFFF;
    unsigned int major = (low >> 8) & 0xFF;
    unsigned int minor = low & 0xFF;

    const char* type = (high == 0xFFFE) ? "vs" : (high == 0xFFFF) ? "ps" : "unknown";

    std::stringstream ss;
    ss << type << "_" << major << "_" << minor;
    return ss.str();
}

void CRenderBackendDX9::CacheDeviceCapsFromD3D()
{
    // 1. Получаем актуальные D3DCAPS9
    D3DCAPS9 caps = {};
    if (m_pDevice)
        m_pDevice->GetDeviceCaps(&caps);

    // 2. Идентификация
    m_DeviceCaps.VendorId = m_AdapterID.VendorId;
    m_DeviceCaps.DeviceId = m_AdapterID.DeviceId;
    m_DeviceCaps.Description = m_AdapterID.Description;

    // 3. Режим рабочего стола
    m_DeviceCaps.DisplayWidth = m_DesktopMode.Width;
    m_DeviceCaps.DisplayHeight = m_DesktopMode.Height;
    m_DeviceCaps.DisplayRefreshRate = m_DesktopMode.RefreshRate;
    m_DeviceCaps.DisplayFormat = D3DFormatToRHI(m_DesktopMode.Format);

    // 4. Остальные параметры
    m_DeviceCaps.MaxTextureWidth = caps.MaxTextureWidth;
    m_DeviceCaps.MaxTextureHeight = caps.MaxTextureHeight;
    m_DeviceCaps.MaxVolumeExtent = caps.MaxVolumeExtent;
    m_DeviceCaps.MaxSimultaneousRTs = caps.NumSimultaneousRTs;
    m_DeviceCaps.HasVertexShader = (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1));
    m_DeviceCaps.HasPixelShader = (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1));
    m_DeviceCaps.VertexShaderMajor = (caps.VertexShaderVersion & 0xFF00) >> 8;
    m_DeviceCaps.VertexShaderMinor = caps.VertexShaderVersion & 0xFF;
    m_DeviceCaps.PixelShaderMajor = (caps.PixelShaderVersion & 0xFF00) >> 8;
    m_DeviceCaps.PixelShaderMinor = caps.PixelShaderVersion & 0xFF;
    m_DeviceCaps.MaxVertexShaderConst = caps.MaxVertexShaderConst;
    m_DeviceCaps.HasDepthStencil = true; // TODO: уточнить через CheckDeviceFormat
    m_DeviceCaps.MaxAnisotropy = caps.MaxAnisotropy;
    m_DeviceCaps.MaxTextureBlendStages = caps.MaxTextureBlendStages;
    m_DeviceCaps.MaxSimultaneousTextures = caps.MaxSimultaneousTextures;
    m_DeviceCaps.HardwareTnL = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
    m_DeviceCaps.SupportsPureDevice = (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0;
    m_DeviceCaps.SupportsNonPow2Textures =
        ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0) ||
        ((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0);

    m_DeviceCaps.VertexCacheMethod = 0;
    m_DeviceCaps.VertexCacheSize = 16;
    if (m_pDevice)
    {
        IDirect3DQuery9* q_vc = nullptr;
        HRESULT hr = m_pDevice->CreateQuery(D3DQUERYTYPE_VCACHE, &q_vc);
        if (SUCCEEDED(hr) && q_vc)
        {
            D3DDEVINFO_VCACHE vc;
            q_vc->Issue(D3DISSUE_END);
            if (SUCCEEDED(q_vc->GetData(&vc, sizeof(vc), D3DGETDATA_FLUSH)))
            {
                m_DeviceCaps.VertexCacheMethod = vc.OptMethod;
                if (vc.OptMethod == 1)
                    m_DeviceCaps.VertexCacheSize = vc.CacheSize;
            }
            q_vc->Release();
        }
    }
}

// Метод GetDeviceCaps
const RHIDeviceCaps& CRenderBackendDX9::GetDeviceCaps() const
{
    return m_DeviceCaps;
}

RHI_END
