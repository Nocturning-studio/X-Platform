#include "stdafx.h"
#include "R_Backend_ResourceBinder.h"
#include "R_Backend.h"
#include "sh_texture.h"
#include "r_constants.h"

// ----------------------------------------------------------------
// Invalidate
// ----------------------------------------------------------------
void CBackendResourceBinder::Invalidate(CRenderBackend& /*backend*/)
{
    m_state = nullptr;
    m_ps = nullptr;
    m_vs = nullptr;
    m_decl = nullptr;
    m_vb = nullptr;
    m_ib = nullptr;
    m_vbStride = 0;
    m_ctable = nullptr;
    m_T = nullptr;

    for (u32 i = 0; i < 16; ++i) m_texturesPS[i] = nullptr;
    for (u32 i = 0; i < 5; ++i) m_texturesVS[i] = nullptr;

#ifdef DEBUG
    m_psName = nullptr;
    m_vsName = nullptr;
#endif
}

// ----------------------------------------------------------------
// State block
// ----------------------------------------------------------------
void CBackendResourceBinder::SetStates(CRenderBackend& backend, IDirect3DStateBlock9* state)
{
    if (m_state != state)
    {
#ifdef DEBUG
        backend.stat.states++;
#endif
        m_state = state;
        if (state)
            state->Apply();   // D3D9 state block apply
    }
}

// ----------------------------------------------------------------
// Pixel Shader
// ----------------------------------------------------------------
void CBackendResourceBinder::SetPixelShader(CRenderBackend& backend, IDirect3DPixelShader9* ps, LPCSTR name)
{
    if (m_ps != ps)
    {
        backend.stat.ps++;
        m_ps = ps;
        D3D_SetPixelShader(backend.GetDevice(), ps);
#ifdef DEBUG
        m_psName = name;
#endif
    }
}

void CBackendResourceBinder::D3D_SetPixelShader(IDirect3DDevice9Ex* device, IDirect3DPixelShader9* ps)
{
    HRESULT hr = device->SetPixelShader(ps);
    VERIFY(SUCCEEDED(hr));
}

// ----------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexShader(CRenderBackend& backend, IDirect3DVertexShader9* vs, LPCSTR name)
{
    if (m_vs != vs)
    {
        backend.stat.vs++;
        m_vs = vs;
        D3D_SetVertexShader(backend.GetDevice(), vs);
#ifdef DEBUG
        m_vsName = name;
#endif
    }
}

void CBackendResourceBinder::D3D_SetVertexShader(IDirect3DDevice9Ex* device, IDirect3DVertexShader9* vs)
{
    HRESULT hr = device->SetVertexShader(vs);
    VERIFY(SUCCEEDED(hr));
}

// ----------------------------------------------------------------
// Vertex Declaration
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexDeclaration(CRenderBackend& backend, IDirect3DVertexDeclaration9* decl)
{
    if (m_decl != decl)
    {
#ifdef DEBUG
        backend.stat.decl++;
#endif
        m_decl = decl;
        D3D_SetVertexDeclaration(backend.GetDevice(), decl);
    }
}

void CBackendResourceBinder::D3D_SetVertexDeclaration(IDirect3DDevice9Ex* device, IDirect3DVertexDeclaration9* decl)
{
    HRESULT hr = device->SetVertexDeclaration(decl);
    VERIFY(SUCCEEDED(hr));
}

// ----------------------------------------------------------------
// Vertex Buffer
// ----------------------------------------------------------------
void CBackendResourceBinder::SetVertexBuffer(CRenderBackend& backend, IDirect3DVertexBuffer9* vb, u32 stride)
{
    if (m_vb != vb || m_vbStride != stride)
    {
#ifdef DEBUG
        backend.stat.vb++;
#endif
        m_vb = vb;
        m_vbStride = stride;
        D3D_SetStreamSource(backend.GetDevice(), 0, vb, stride);
    }
}

void CBackendResourceBinder::D3D_SetStreamSource(IDirect3DDevice9Ex* device, u32 stream, IDirect3DVertexBuffer9* vb, u32 stride)
{
    HRESULT hr = device->SetStreamSource(stream, vb, 0, stride);
    VERIFY(SUCCEEDED(hr));
}

// ----------------------------------------------------------------
// Index Buffer
// ----------------------------------------------------------------
void CBackendResourceBinder::SetIndexBuffer(CRenderBackend& backend, IDirect3DIndexBuffer9* ib)
{
    if (m_ib != ib)
    {
#ifdef DEBUG
        backend.stat.ib++;
#endif
        m_ib = ib;
        D3D_SetIndices(backend.GetDevice(), ib);
    }
}

void CBackendResourceBinder::D3D_SetIndices(IDirect3DDevice9Ex* device, IDirect3DIndexBuffer9* ib)
{
    HRESULT hr = device->SetIndices(ib);
    VERIFY(SUCCEEDED(hr));
}

// ----------------------------------------------------------------
// Constant Table (with handler processing)
// ----------------------------------------------------------------
void CBackendResourceBinder::SetConstantTable(CRenderBackend& backend, R_constant_table* ctable, R_transforms& transforms)
{
    if (m_ctable == ctable)
        return;

    m_ctable = ctable;
    transforms.unmap();

    if (!ctable)
        return;

    // process constant-loaders
    R_constant_table::c_table::iterator it = ctable->table.begin();
    R_constant_table::c_table::iterator end = ctable->table.end();
    for (; it != end; ++it)
    {
        R_constant* C = &**it;
        if (C->handler)
            C->handler->setup(C);
    }
}

// ----------------------------------------------------------------
// Textures
// ----------------------------------------------------------------
void CBackendResourceBinder::SetTextures(CRenderBackend& backend, STextureList* T)
{
    if (m_T == T)
        return;
    m_T = T;

    u32 last_ps = 0;
    u32 last_vs = 0;

    if (!T)
        return;

    STextureList::iterator it = T->begin();
    STextureList::iterator end = T->end();
    for (; it != end; ++it)
    {
        std::pair<u32, ref_texture>& loader = *it;
        u32 load_id = loader.first;
        CTexture* load_surf = &*loader.second;

        if (load_id < 256) // pixel stage
        {
            if (load_id > last_ps) last_ps = load_id;
            if (m_texturesPS[load_id] != load_surf)
            {
                m_texturesPS[load_id] = load_surf;
#ifdef DEBUG
                backend.stat.textures++;
#endif
                if (load_surf)
                    load_surf->bind(load_id); // bind internally calls D3D
                else
                    D3D_SetTexture(backend.GetDevice(), load_id, nullptr);
            }
        }
        else // vertex stage (dmap or custom)
        {
            u32 load_id_remapped = load_id - 256;
            if (load_id_remapped > last_vs) last_vs = load_id_remapped;
            if (m_texturesVS[load_id_remapped] != load_surf)
            {
                m_texturesVS[load_id_remapped] = load_surf;
#ifdef DEBUG
                backend.stat.textures++;
#endif
                if (load_surf)
                    load_surf->bind(load_id);
                else
                    D3D_SetTexture(backend.GetDevice(), load_id, nullptr);
            }
        }
    }

    // clear remaining pixel stages
    for (++last_ps; last_ps < 16; ++last_ps)
    {
        if (m_texturesPS[last_ps] != nullptr)
        {
            m_texturesPS[last_ps] = nullptr;
            D3D_SetTexture(backend.GetDevice(), last_ps, nullptr);
        }
    }

    // clear remaining vertex stages
    for (++last_vs; last_vs < 5; ++last_vs)
    {
        if (m_texturesVS[last_vs] != nullptr)
        {
            m_texturesVS[last_vs] = nullptr;
            D3D_SetTexture(backend.GetDevice(), last_vs + 256, nullptr);
        }
    }
}

CTexture* CBackendResourceBinder::GetActiveTexture(u32 stage) const
{
    if (stage >= 256)
        return m_texturesVS[stage - 256];
    else
        return m_texturesPS[stage];
}

void CBackendResourceBinder::D3D_SetTexture(IDirect3DDevice9Ex* device, u32 stage, CTexture* tex)
{
    if (tex)
        tex->bind(stage);   // calls device->SetTexture internally
    else
    {
        HRESULT hr = device->SetTexture(stage, nullptr);
        VERIFY(SUCCEEDED(hr));
    }
}
