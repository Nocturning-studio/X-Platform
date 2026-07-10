#pragma once

#include "R_Backend_StateCache.h"
#include "R_Backend_ResourceBinder.h"
#include "R_Backend_Data_Streams.h"
#include "r_constants_cache.h"
#include "r_backend_transform.h"
#include "r_backend_tree.h"
#include "fvf.h"
#include "../xrRHI/xrRHI.h"
#include <d3dx9.h>

const u32 CULL_BACKFACE = D3DCULL_CCW;
const u32 CULL_FRONTFACE = D3DCULL_CW;
const u32 CULL_DISABLE = D3DCULL_NONE;

const u32 CLEAR_RENDERTARGET = D3DCLEAR_TARGET;
const u32 CLEAR_ZBUFFER = D3DCLEAR_ZBUFFER;
const u32 CLEAR_STENCIL = D3DCLEAR_STENCIL;

struct R_statistics_element
{
    u32 verts, dips;
    ICF void add(u32 _verts)
    {
        verts += _verts;
        dips++;
    }
};

struct R_statistics
{
    R_statistics_element s_static;
    R_statistics_element s_flora;
    R_statistics_element s_flora_lods;
    R_statistics_element s_details;
    R_statistics_element s_ui;
    R_statistics_element s_dynamic;
    R_statistics_element s_dynamic_sw;
    R_statistics_element s_dynamic_inst;
    R_statistics_element s_dynamic_1B;
    R_statistics_element s_dynamic_2B;
};

class ENGINE_API CRenderBackend
{
public:
    // D3D / RHI
    IDirect3D9Ex* m_pD3D;
    IDirect3DDevice9Ex* m_pDevice;
    IDirect3DSurface9* m_pBaseRT;
    IDirect3DSurface9* m_pBaseZB;
    D3DPRESENT_PARAMETERS m_DevPP;

    xrRHI::IRenderBackend* m_pRHI;
    HINSTANCE m_hRHI_DLL;

    // Dynamic streams (will be refactored later)
    VertexStream Vertex;
    IndexStream Index;

    IDirect3DIndexBuffer9* QuadIB;
    IDirect3DIndexBuffer9* old_QuadIB;

    R_transforms transforms;
    R_tree tree;

    ref_geom m_viewport;

    // New state cache and resource binder
    CBackendStateCache m_stateCache;
    CBackendResourceBinder m_resBinder;

private:
    ALIGN(16) R_constants constants; // will be moved to CConstantManager later

    void Invalidate();

public:
    struct _stats
    {
        u32 polys;
        u32 verts;
        u32 calls;
        u32 vs;
        u32 ps;
#ifdef DEBUG
        u32 decl;
        u32 vb;
        u32 ib;
        u32 states;
        u32 textures;
#endif
        u32 transforms;
        u32 target_rt;
        u32 target_zb;

        R_statistics r;
    } stat;

public:
    CRenderBackend();
    ~CRenderBackend();

    // Device access
    DEPRECATED IDirect3DDevice9Ex* GetDevice() const { return m_pDevice; }
    DEPRECATED IDirect3D9Ex* GetD3D() const { return m_pD3D; }
    IDirect3DSurface9* GetBaseRT() const { return m_pBaseRT; }
    IDirect3DSurface9* GetBaseZB() const { return m_pBaseZB; }
    xrRHI::IRenderBackend* GetRHI() const { return m_pRHI; }

    // Initialization
    void Create(HWND hWnd);
    void Destroy();
    void Reset();
    bool NeedReset();

    void selectResolution(u32& w, u32& h, BOOL bWindowed);
    u32  selectPresentInterval();

    // Render state save/restore (delegates to cache)
    void SaveRenderState();
    void RestoreRenderState();

    // Active texture info
    IC CTexture* get_ActiveTexture(u32 stage)
    {
        return m_resBinder.GetActiveTexture(stage);
    }
    IC R_constant_array& get_ConstantCache_Vertex() { return constants.a_vertex; }
    IC R_constant_array& get_ConstantCache_Pixel() { return constants.a_pixel; }

    // Transform API (implementations remain in R_Backend_Runtime.h or .cpp)
    IC void set_transform_world(const fmat4x4& M);
    IC void set_transform_view(const fmat4x4& M);
    IC void set_transform_project(const fmat4x4& M);
    IC const fmat4x4& get_transform_world();
    IC const fmat4x4& get_transform_view();
    IC const fmat4x4& get_transform_project();

    // --- Pipeline state (delegated to m_stateCache) ---
    IC void setRenderTarget(IDirect3DSurface9* RT, u32 ID = 0)
    {
        m_stateCache.SetRenderTarget(*this, RT, ID);
    }
    IC void setDepthBuffer(IDirect3DSurface9* ZB)
    {
        m_stateCache.SetDepthStencil(*this, ZB);
    }
    IC void set_Stencil(u32 _enable, u32 _func = D3DCMP_ALWAYS, u32 _ref = 0x00, u32 _mask = 0x00,
        u32 _writemask = 0x00, u32 _fail = D3DSTENCILOP_KEEP, u32 _pass = D3DSTENCILOP_KEEP,
        u32 _zfail = D3DSTENCILOP_KEEP)
    {
        m_stateCache.SetStencil(GetDevice(), _enable, _func, _ref, _mask, _writemask, _fail, _pass, _zfail);
    }
    IC void set_ColorWriteEnable(u32 _mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
        D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA)
    {
        m_stateCache.SetColorWriteEnable(GetDevice(), _mask);
    }
    IC void set_ZWriteEnable(bool state)
    {
        m_stateCache.SetZWriteEnable(GetDevice(), state);
    }
    IC void set_CullMode(u32 _mode)
    {
        m_stateCache.SetCullMode(GetDevice(), _mode);
    }
    IC void set_Scissor(Irect* rect = NULL)
    {
        m_stateCache.SetScissor(GetDevice(), (const RECT*)rect);
    }

    ICF void SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
    {
        m_stateCache.SetRawRenderState(GetDevice(), State, Value);
    }

    // Blend helpers (delegated)
    void set_Blend(BOOL enable, D3DBLEND src = D3DBLEND_ONE, D3DBLEND dest = D3DBLEND_ZERO);
    void set_Blend_Alpha();
    void set_Blend_Add();
    void set_Blend_Multiply();
    void set_Blend_Default();
    void set_Blend_Subtract();
    void set_Blend_Screen();
    void set_Blend_LightAdd();
    void set_Blend_ColorAdd();
    void set_BlendEx(BOOL enable, D3DBLEND src, D3DBLEND dest, D3DBLENDOP op = D3DBLENDOP_ADD);
    BOOL get_BlendState() const;
    D3DBLEND get_SrcBlend() const;
    D3DBLEND get_DstBlend() const;

    // Anisotropy (will be moved to sampler state manager later)
    void enable_anisotropy_filtering();
    void disable_anisotropy_filtering();
    void set_anisotropy_filtering(int max_anisothropy);

    // --- Resource binding (delegated to m_resBinder) ---
    IC void set_Constants(R_constant_table* C)
    {
        m_resBinder.SetConstantTable(*this, C, transforms);
    }
    IC void set_Constants(ref_ctable& CTable) { set_Constants(&*CTable); }

    void set_Textures(STextureList* T);
    IC void set_Textures(ref_texture_list& TexList) { set_Textures(&*TexList); }

    IC void set_Element(ShaderElement* S, u32 pass = 0)
    {
        SPass& P = *(S->passes[pass]);
        set_States(P.state);
        set_Pixel_Shader(P.ps);
        set_Vertex_Shader(P.vs);
        set_Constants(P.constants);
        set_Textures(P.T);
    }
    IC void set_Element(ref_selement& S, u32 pass = 0) { set_Element(&*S, pass); }

    IC void set_Shader(Shader* S, u32 pass = 0) { set_Element(S->E[0], pass); }
    IC void set_Shader(ref_shader& S, u32 pass = 0) { set_Shader(&*S, pass); }

    ICF void set_States(IDirect3DStateBlock9* _state) { m_resBinder.SetStates(*this, _state); }
    ICF void set_States(ref_state& _state) { set_States(_state->state); }

    ICF void set_Format(IDirect3DVertexDeclaration9* _decl) { m_resBinder.SetVertexDeclaration(*this, _decl); }

    ICF void set_Pixel_Shader(IDirect3DPixelShader9* _ps, LPCSTR _n = 0) { m_resBinder.SetPixelShader(*this, _ps, _n); }
    ICF void set_Pixel_Shader(ref_ps& _ps) { set_Pixel_Shader(_ps->sh, _ps->cName.c_str()); }

    ICF void set_Vertex_Shader(IDirect3DVertexShader9* _vs, LPCSTR _n = 0) { m_resBinder.SetVertexShader(*this, _vs, _n); }
    ICF void set_Vertex_Shader(ref_vs& _vs) { set_Vertex_Shader(_vs->sh, _vs->cName.c_str()); }

    ICF void set_Vertices(IDirect3DVertexBuffer9* _vb, u32 _vb_stride) { m_resBinder.SetVertexBuffer(*this, _vb, _vb_stride); }
    ICF void set_Indices(IDirect3DIndexBuffer9* _ib) { m_resBinder.SetIndexBuffer(*this, _ib); }

    ICF void set_Geometry(SGeometry* _geom)
    {
        set_Format(_geom->dcl._get()->dcl);
        set_Vertices(_geom->vb, _geom->vb_stride);
        set_Indices(_geom->ib);
    }
    ICF void set_Geometry(ref_geom& _geom) { set_Geometry(&*_geom); }

    // Constant setters (still using internal R_constants, will be extracted to CConstantManager)
    ICF ref_constant get_Constant(LPCSTR n)
    {
        R_constant_table* ctable = m_resBinder.GetConstantTable();
        return ctable ? ctable->get(n) : 0;
    }
    ICF ref_constant get_Constant(shared_str& n)
    {
        R_constant_table* ctable = m_resBinder.GetConstantTable();
        return ctable ? ctable->get(n) : 0;
    }

    ICF void set_Constant(R_constant* Const, const fmat4x4& A) { if (Const) constants.set(Const, A); }
    ICF void set_Constant(R_constant* Const, const fvec4& A) { if (Const) constants.set(Const, A); }
    ICF void set_Constant(R_constant* Const, float x, float y = NULL, float z = NULL, float w = NULL) { if (Const) constants.set(Const, x, y, z, w); }
    ICF void set_Array_Constant(R_constant* Const, u32 e, const fmat4x4& A) { if (Const) constants.seta(Const, e, A); }
    ICF void set_Array_Constant(R_constant* Const, u32 e, const fvec4& A) { if (Const) constants.seta(Const, e, A); }
    ICF void set_Array_Constant(R_constant* Const, u32 e, float x, float y, float z, float w) { if (Const) constants.seta(Const, e, x, y, z, w); }

    // Slow lookups via ctable
    ICF void set_Constant(LPCSTR n, const fmat4x4& A) { set_Constant(get_Constant(n)._get(), A); }
    ICF void set_Constant(LPCSTR n, const fvec4& A) { set_Constant(get_Constant(n)._get(), A); }
    ICF void set_Constant(LPCSTR n, float x) { set_Constant(get_Constant(n)._get(), x, 0, 0, 0); }
    ICF void set_Constant(LPCSTR n, float x, float y) { set_Constant(get_Constant(n)._get(), x, y, 0, 0); }
    ICF void set_Constant(LPCSTR n, float x, float y, float z) { set_Constant(get_Constant(n)._get(), x, y, z, 0); }
    ICF void set_Constant(LPCSTR n, float x, float y, float z, float w) { set_Constant(get_Constant(n)._get(), x, y, z, w); }
    ICF void set_Array_Constant(LPCSTR n, u32 e, const fmat4x4& A) { set_Array_Constant(get_Constant(n)._get(), e, A); }
    ICF void set_Array_Constant(LPCSTR n, u32 e, const fvec4& A) { set_Array_Constant(get_Constant(n)._get(), e, A); }
    ICF void set_Array_Constant(LPCSTR n, u32 e, float x, float y, float z, float w) { set_Array_Constant(get_Constant(n)._get(), e, x, y, z, w); }

    ICF void set_Constant(shared_str& n, const fmat4x4& A) { set_Constant(get_Constant(n)._get(), A); }
    ICF void set_Constant(shared_str& n, const fvec4& A) { set_Constant(get_Constant(n)._get(), A); }
    ICF void set_Constant(shared_str& n, const fvec3& A) { set_Constant(get_Constant(n)._get(), fvec4().set(A.x, A.y, A.z, 0.0f)); }
    ICF void set_Constant(shared_str& n, float x, float y, float z, float w) { set_Constant(get_Constant(n)._get(), x, y, z, w); }
    ICF void set_Array_Constant(shared_str& n, u32 e, const fmat4x4& A) { set_Array_Constant(get_Constant(n)._get(), e, A); }
    ICF void set_Array_Constant(shared_str& n, u32 e, const fvec4& A) { set_Array_Constant(get_Constant(n)._get(), e, A); }
    ICF void set_Array_Constant(shared_str& n, u32 e, float x, float y, float z, float w) { set_Array_Constant(get_Constant(n)._get(), e, x, y, z, w); }

    // Drawing
    ICF void Apply(u32 countV, u32 PC);
    ICF void Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
    ICF void Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC);

    // Device & frame
    void CreateQuadIB();
    void OnFrameBegin();
    void Present();
    void OnFrameEnd();
    void OnDeviceCreate();
    void OnDeviceDestroy();
    void DeleteResources();
    void reset_begin();
    void reset_end();

    // Debug (temporary direct D3D calls, will be moved to CDebugRenderer)
    void dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc);
    void dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
    IC void dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2) { CHK_DX(m_pDevice->SetRenderState(p1, p2)); }
    IC void dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value) { CHK_DX(m_pDevice->SetSamplerState(sampler, type, value)); }
    void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt);
    void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt);
    IC void dbg_DrawAABB(fvec3& Translation, float sx, float sy, float sz, u32 Color);
    void dbg_DrawOBB(fmat4x4& T, fvec3& half_dim, u32 C);
    IC void dbg_DrawTRI(fmat4x4& Transform, fvec3* p, u32 Color) { dbg_DrawTRI(Transform, p[0], p[1], p[2], Color); }
    void dbg_DrawTRI(fmat4x4& T, fvec3& p1, fvec3& p2, fvec3& p3, u32 C);
    void dbg_DrawLINE(fmat4x4& T, fvec3& p1, fvec3& p2, u32 C);
    void dbg_DrawEllipse(fmat4x4& T, u32 C);

    // Render target / viewport helpers
    void u_compute_texgen_screen(fmat4x4& dest);
    void set_viewport_geometry(u32 w, u32 h, ref_geom geometry, u32& vOffset);
    void set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);
    void set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2 = NULL, IDirect3DSurface9* _3 = NULL, IDirect3DSurface9* _4 = NULL);
    void set_Depth_Buffer(IDirect3DSurface9* zb);
    void clear_Depth_Buffer(IDirect3DSurface9* zb);
    void set_viewport_geometry(u32 w, u32 h, u32& vOffset);
    void set_viewport_geometry(ref_geom geometry, u32& vOffset);
    void set_viewport_geometry(u32& vOffset);
    void render_viewport_geometry(u32 w, u32 h);
    void RenderViewportSurface();
    void RenderViewportSurface(u32 w, u32 h, IDirect3DSurface9* _1, IDirect3DSurface9* zb = NULL);
    void RenderViewportSurface(IDirect3DSurface9* _1);
    void RenderViewportSurface(const ref_rt& _1, IDirect3DSurface9* zb = NULL);
    void RenderViewportSurface(u32 w, u32 h, const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);
    void RenderToMipLevel(ref_rt target, u32 mip_level);
    void RenderToMipLevel(ref_rt target, u32 mip_level, ShaderElement* shader, u32 pass);
    void GenerateMipChain(ref_rt source, ref_rt mip_chain, ShaderElement* downsample_shader, u32 pass = 0);
    void CopyViewportSurface(ref_rt source, ref_rt destination);
    void CopyViewportSurface(ref_rt source, ref_rt destination, D3DTEXTUREFILTERTYPE filter);
    void CopyViewportSurface(ref_rt source, RECT src_rect, ref_rt destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter);
    void CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination);
    void CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination, D3DTEXTUREFILTERTYPE filter);
    void CopySurface(IDirect3DSurface9* source, RECT src_rect, IDirect3DSurface9* destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter);
    ICF void Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
    ICF void ClearTexture(const ref_rt& _1, u32 color = color_rgba(0, 0, 0, 0));
    ICF void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, u32 color = color_rgba(0, 0, 0, 0));
    ICF void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, u32 color = color_rgba(0, 0, 0, 0));
    ICF void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL, u32 color = color_rgba(0, 0, 0, 0));
};

extern ENGINE_API CRenderBackend RenderBackend;

inline xrRHI::IRenderBackend* RHI()
{
    return RenderBackend.GetRHI();
}

#include "R_Backend.inl"

#ifndef _EDITOR
#include "D3DUtils.h"
#endif
