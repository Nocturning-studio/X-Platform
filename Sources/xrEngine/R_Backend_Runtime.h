#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "sh_texture.h"
#include "R_Backend_RenderTarget.h"

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
IC void CRenderBackend::set_transform_world(const fmat4x4& Matrix)
{
	transforms.set_World(Matrix);
}
IC void CRenderBackend::set_transform_view(const fmat4x4& Matrix)
{
	transforms.set_View(Matrix);
}
IC void CRenderBackend::set_transform_project(const fmat4x4& Matrix)
{
	transforms.set_Project(Matrix);
}
IC const fmat4x4& CRenderBackend::get_transform_world()
{
	return transforms.get_World();
}
IC const fmat4x4& CRenderBackend::get_transform_view()
{
	return transforms.get_View();
}
IC const fmat4x4& CRenderBackend::get_transform_project()
{
	return transforms.get_Project();
}

IC void CRenderBackend::setRenderTarget(IDirect3DSurface9* RT, u32 ID)
{
	m_stateCache.SetRenderTarget(*this, RT, ID);
}

IC void CRenderBackend::setDepthBuffer(IDirect3DSurface9* ZB)
{
	m_stateCache.SetDepthStencil(*this, ZB);
}

ICF void CRenderBackend::set_States(IDirect3DStateBlock9* _state)
{
	if (state != _state)
	{
#ifdef DEBUG
		stat.states++;
#endif
		state = _state;
		state->Apply();
	}
}

ICF void CRenderBackend::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	m_stateCache.SetRawRenderState(GetDevice(), State, Value);
}

IC void CRenderBackend::set_Constants(R_constant_table* ConstTable)
{
	// caching
	if (ctable == ConstTable)
		return;

	ctable = ConstTable;
	transforms.unmap();

	if (0 == ConstTable)
		return;

	// process constant-loaders
	R_constant_table::c_table::iterator it = ConstTable->table.begin();
	R_constant_table::c_table::iterator end = ConstTable->table.end();
	for (; it != end; it++)
	{
		R_constant* Constant = &**it;
		if (Constant->handler)
			Constant->handler->setup(Constant);
	}
}

IC void CRenderBackend::set_Element(ShaderElement* S, u32 pass)
{
	SPass& P = *(S->passes[pass]);
	set_States(P.state);
	set_Pixel_Shader(P.ps);
	set_Vertex_Shader(P.vs);
	set_Constants(P.constants);
	set_Textures(P.T);
}

ICF void CRenderBackend::set_Format(IDirect3DVertexDeclaration9* _decl)
{
	if (decl != _decl)
	{
#ifdef DEBUG
		stat.decl++;
#endif
		decl = _decl;
		CHK_DX(RenderBackend.GetDevice()->SetVertexDeclaration(decl));
	}
}

ICF void CRenderBackend::set_Pixel_Shader(IDirect3DPixelShader9* _ps, LPCSTR _n)
{
	if (ps != _ps)
	{
		stat.ps++;
		ps = _ps;
		CHK_DX(RenderBackend.GetDevice()->SetPixelShader(ps));
#ifdef DEBUG
		ps_name = _n;
#endif
	}
}

ICF void CRenderBackend::set_Vertex_Shader(IDirect3DVertexShader9* _vs, LPCSTR _n)
{
	if (vs != _vs)
	{
		stat.vs++;
		vs = _vs;
		CHK_DX(RenderBackend.GetDevice()->SetVertexShader(vs));
#ifdef DEBUG
		vs_name = _n;
#endif
	}
}

ICF void CRenderBackend::set_Vertices(IDirect3DVertexBuffer9* _vb, u32 _vb_stride)
{
	if ((vb != _vb) || (vb_stride != _vb_stride))
	{
#ifdef DEBUG
		stat.vb++;
#endif
		vb = _vb;
		vb_stride = _vb_stride;
		CHK_DX(RenderBackend.GetDevice()->SetStreamSource(0, vb, 0, vb_stride));
	}
}

ICF void CRenderBackend::set_Indices(IDirect3DIndexBuffer9* _ib)
{
	if (ib != _ib)
	{
#ifdef DEBUG
		stat.ib++;
#endif
		ib = _ib;
		CHK_DX(RenderBackend.GetDevice()->SetIndices(ib));
	}
}

ICF void CRenderBackend::Apply(u32 countV, u32 PC)
{
	//OPTICK_EVENT("CRenderBackend::Apply");

	stat.calls++;
	stat.verts += countV;
	stat.polys += PC;
	constants.flush();
}

ICF void CRenderBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
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

ICF void CRenderBackend::set_Shader(Shader* S, u32 pass)
{
	set_Element(S->E[0], pass);
}

IC void CRenderBackend::set_Geometry(SGeometry* _geom)
{
	set_Format(_geom->dcl._get()->dcl);
	set_Vertices(_geom->vb, _geom->vb_stride);
	set_Indices(_geom->ib);
}

IC void CRenderBackend::set_Scissor(Irect* R)
{
	m_stateCache.SetScissor(GetDevice(), (const RECT*)R);
}

IC void CRenderBackend::set_Stencil(u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass, u32 _zfail)
{
	m_stateCache.SetStencil(GetDevice(), _enable, _func, _ref, _mask, _writemask, _fail, _pass, _zfail);
}

IC void CRenderBackend::set_ColorWriteEnable(u32 _mask)
{
	m_stateCache.SetColorWriteEnable(GetDevice(), _mask);
}
IC void CRenderBackend::set_ZWriteEnable(bool write_state)
{
	m_stateCache.SetZWriteEnable(GetDevice(), write_state);
}
ICF void CRenderBackend::set_CullMode(u32 _mode)
{
	m_stateCache.SetCullMode(GetDevice(), _mode);
}

#endif
