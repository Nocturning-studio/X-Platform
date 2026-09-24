#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "R_Backend_ResourceBinder.h"
#include "sh_texture.h"
#include "R_Backend_RenderTarget.h"
#include "R_Backend_transform.h"

// ------------------------------------------------------------
// Transform helpers
// ------------------------------------------------------------
IC void R_transforms::set_c_World(R_constant* C)
{
    c_World = C;
    RenderBackend.SetConstant(C, m_World);
};
IC void R_transforms::set_c_InvWorld(R_constant* C)
{
    c_InvWorld = C;
    apply_InvWorld();
};
IC void R_transforms::set_c_View(R_constant* C)
{
    c_View = C;
    RenderBackend.SetConstant(C, m_View);
};
IC void R_transforms::set_c_Project(R_constant* C)
{
    c_Project = C;
    RenderBackend.SetConstant(C, m_Project);
};
IC void R_transforms::set_c_WorldView(R_constant* C)
{
    c_WorldView = C;
    RenderBackend.SetConstant(C, m_WorldView);
};
IC void R_transforms::set_c_ViewProject(R_constant* C)
{
    c_ViewProject = C;
    RenderBackend.SetConstant(C, m_ViewProject);
};
IC void R_transforms::set_c_WorldViewProject(R_constant* C)
{
    c_WorldViewProject = C;
    RenderBackend.SetConstant(C, m_WorldViewProject);
};

IC void CRenderBackendFacade::SetTransformWorld(const fmat4x4& Matrix) { transforms.set_World(Matrix); }
IC void CRenderBackendFacade::SetTransformView(const fmat4x4& Matrix) { transforms.set_View(Matrix); }
IC void CRenderBackendFacade::SetTransformProject(const fmat4x4& Matrix) { transforms.set_Project(Matrix); }
IC const fmat4x4& CRenderBackendFacade::GetTransformWorld() { return transforms.get_World(); }
IC const fmat4x4& CRenderBackendFacade::GetTransformView() { return transforms.get_View(); }
IC const fmat4x4& CRenderBackendFacade::GetTransformProject() { return transforms.get_Project(); }

// ------------------------------------------------------------
// Apply / Render
// ------------------------------------------------------------
ICF void CRenderBackendFacade::Apply(u32 countV, u32 PC)
{
    stat.calls++;
    stat.verts += countV;
    stat.polys += PC;
    m_constantMgr.Flush();
}

ICF void CRenderBackendFacade::Render(D3DPRIMITIVETYPE PrimitiveType, u32 baseV, u32 startV,
    u32 countV, u32 startI, u32 PC)
{
    Apply(countV, PC);
    CHK_DX(RenderBackend.GetDevice()->DrawIndexedPrimitive(PrimitiveType, baseV, startV, countV, startI, PC));
}

ICF void CRenderBackendFacade::Render(D3DPRIMITIVETYPE PrimitiveType, u32 startV, u32 PC)
{
    stat.calls++;
    stat.verts += 3 * PC;
    stat.polys += PC;
    m_constantMgr.Flush();
    CHK_DX(RenderBackend.GetDevice()->DrawPrimitive(PrimitiveType, startV, PC));
}

ICF void CRenderBackendFacade::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    CHK_DX(RenderBackend.GetDevice()->Clear(Count, pRects, Flags, Color, Z, Stencil));
}

ICF void CRenderBackendFacade::ClearTexture(const ref_rt& rt_1, u32 color)
{
    SetRenderTarget(rt_1, NULL, NULL, NULL);
    Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

ICF void CRenderBackendFacade::ClearTexture(const ref_rt& rt_1, const ref_rt& rt_2, u32 color)
{
    SetRenderTarget(rt_1, rt_2, NULL, NULL);
    Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

ICF void CRenderBackendFacade::ClearTexture(const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, u32 color)
{
    SetRenderTarget(rt_1, rt_2, rt_3, NULL);
    Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

ICF void CRenderBackendFacade::ClearTexture(const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, const ref_rt& rt_4, u32 color)
{
    SetRenderTarget(rt_1, rt_2, rt_3, rt_4);
    Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

#endif
