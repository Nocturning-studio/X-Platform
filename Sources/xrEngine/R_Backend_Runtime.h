#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "R_Backend_StateCache.h"
#include "R_Backend_ResourceBinder.h"
#include "sh_texture.h"
#include "R_Backend_RenderTarget.h"

// ------------------------------------------------------------
// Transform helpers
// ------------------------------------------------------------
IC void R_transforms::set_c_World(R_constant* C)
{
    c_World = C;
    RenderBackend.set_Constant(C, m_World);
};
IC void R_transforms::set_c_InvWorld(R_constant* C)
{
    c_InvWorld = C;
    apply_InvWorld();
};
IC void R_transforms::set_c_View(R_constant* C)
{
    c_View = C;
    RenderBackend.set_Constant(C, m_View);
};
IC void R_transforms::set_c_Project(R_constant* C)
{
    c_Project = C;
    RenderBackend.set_Constant(C, m_Project);
};
IC void R_transforms::set_c_WorldView(R_constant* C)
{
    c_WorldView = C;
    RenderBackend.set_Constant(C, m_WorldView);
};
IC void R_transforms::set_c_ViewProject(R_constant* C)
{
    c_ViewProject = C;
    RenderBackend.set_Constant(C, m_ViewProject);
};
IC void R_transforms::set_c_WorldViewProject(R_constant* C)
{
    c_WorldViewProject = C;
    RenderBackend.set_Constant(C, m_WorldViewProject);
};

IC void CRenderBackend::set_transform_world(const fmat4x4& Matrix) { transforms.set_World(Matrix); }
IC void CRenderBackend::set_transform_view(const fmat4x4& Matrix) { transforms.set_View(Matrix); }
IC void CRenderBackend::set_transform_project(const fmat4x4& Matrix) { transforms.set_Project(Matrix); }
IC const fmat4x4& CRenderBackend::get_transform_world() { return transforms.get_World(); }
IC const fmat4x4& CRenderBackend::get_transform_view() { return transforms.get_View(); }
IC const fmat4x4& CRenderBackend::get_transform_project() { return transforms.get_Project(); }

// ------------------------------------------------------------
// Apply / Render
// ------------------------------------------------------------
ICF void CRenderBackend::Apply(u32 countV, u32 PC)
{
    stat.calls++;
    stat.verts += countV;
    stat.polys += PC;
    constants.flush();               // will be delegated later to CConstantManager
}

ICF void CRenderBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 baseV, u32 startV,
    u32 countV, u32 startI, u32 PC)
{
    Apply(countV, PC);
    CHK_DX(RenderBackend.GetDevice()->DrawIndexedPrimitive(PrimitiveType, baseV, startV, countV, startI, PC));
}

ICF void CRenderBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 startV, u32 PC)
{
    stat.calls++;
    stat.verts += 3 * PC;
    stat.polys += PC;
    constants.flush();
    CHK_DX(RenderBackend.GetDevice()->DrawPrimitive(PrimitiveType, startV, PC));
}

#endif
