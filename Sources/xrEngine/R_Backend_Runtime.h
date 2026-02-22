#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "../xrEngine/optick_include.h"
#include "sh_texture.h"
#include "sh_matrix.h"
#include "sh_constant.h"
#include "R_Backend_RenderTarget.h"

IC void R_transforms::set_c_World(R_constant* C)
{
	c_World = C;
	RenderBackendLegacy.set_Constant(C, m_World);
};
IC void R_transforms::set_c_InvWorld(R_constant* C)
{
	c_InvWorld = C;
	apply_InvWorld();
};
IC void R_transforms::set_c_View(R_constant* C)
{
	c_View = C;
	RenderBackendLegacy.set_Constant(C, m_View);
};
IC void R_transforms::set_c_Project(R_constant* C)
{
	c_Project = C;
	RenderBackendLegacy.set_Constant(C, m_Project);
};
IC void R_transforms::set_c_WorldView(R_constant* C)
{
	c_WorldView = C;
	RenderBackendLegacy.set_Constant(C, m_WorldView);
};
IC void R_transforms::set_c_ViewProject(R_constant* C)
{
	c_ViewProject = C;
	RenderBackendLegacy.set_Constant(C, m_ViewProject);
};
IC void R_transforms::set_c_WorldViewProject(R_constant* C)
{
	c_WorldViewProject = C;
	RenderBackendLegacy.set_Constant(C, m_WorldViewProject);
};
IC void CBackend::set_transform_world(const float4x4& Matrix)
{
	transforms.set_World(Matrix);
}
IC void CBackend::set_transform_view(const float4x4& Matrix)
{
	transforms.set_View(Matrix);
}
IC void CBackend::set_transform_project(const float4x4& Matrix)
{
	transforms.set_Project(Matrix);
}
IC const float4x4& CBackend::get_transform_world()
{
	return transforms.get_World();
}
IC const float4x4& CBackend::get_transform_view()
{
	return transforms.get_View();
}
IC const float4x4& CBackend::get_transform_project()
{
	return transforms.get_Project();
}

IC void CBackend::setRenderTarget(IDirect3DSurface9* RT, u32 ID)
{		
	if (RT != pRT[ID])
	{
		stat.target_rt++;
		pRT[ID] = RT;
		CHK_DX(HW.GetDevice()->SetRenderTarget(ID, RT));
	}
}

IC void CBackend::setDepthBuffer(IDirect3DSurface9* ZB)
{
	if (ZB != pZB)
	{
		stat.target_zb++;
		pZB = ZB;
		CHK_DX(HW.GetDevice()->SetDepthStencilSurface(ZB));
	}
}

ICF void CBackend::set_States(IDirect3DStateBlock9* _state)
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

ICF void CBackend::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	CHK_DX(HW.GetDevice()->SetRenderState(State, Value));
};

#ifdef _EDITOR
IC void CBackend::set_Matrices(SMatrixList* _M)
{
	if (M != _M)
	{
		M = _M;
		if (M)
		{
			for (u32 it = 0; it < M->size(); it++)
			{
				CMatrix* mat = &*((*M)[it]);
				if (mat && matrices[it] != mat)
				{
					matrices[it] = mat;
					mat->Calculate();
					set_transform(D3DTS_TEXTURE0 + it, mat->transform);
					stat.matrices++;
				}
			}
		}
	}
}
#endif

IC void CBackend::set_Constants(R_constant_table* ConstTable)
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

IC void CBackend::set_Element(ShaderElement* S, u32 pass)
{
	SPass& P = *(S->passes[pass]);
	set_States(P.state);
	set_Pixel_Shader(P.ps);
	set_Vertex_Shader(P.vs);
	set_Constants(P.constants);
	set_Textures(P.T);
#ifdef _EDITOR
	set_Matrices(P.M);
#endif
}

ICF void CBackend::set_Format(IDirect3DVertexDeclaration9* _decl)
{
	if (decl != _decl)
	{
#ifdef DEBUG
		stat.decl++;
#endif
		decl = _decl;
		CHK_DX(HW.GetDevice()->SetVertexDeclaration(decl));
	}
}

ICF void CBackend::set_Pixel_Shader(IDirect3DPixelShader9* _ps, LPCSTR _n)
{
	if (ps != _ps)
	{
		stat.ps++;
		ps = _ps;
		CHK_DX(HW.GetDevice()->SetPixelShader(ps));
#ifdef DEBUG
		ps_name = _n;
#endif
	}
}

ICF void CBackend::set_Vertex_Shader(IDirect3DVertexShader9* _vs, LPCSTR _n)
{
	if (vs != _vs)
	{
		stat.vs++;
		vs = _vs;
		CHK_DX(HW.GetDevice()->SetVertexShader(vs));
#ifdef DEBUG
		vs_name = _n;
#endif
	}
}

ICF void CBackend::set_Vertices(IDirect3DVertexBuffer9* _vb, u32 _vb_stride)
{
	if ((vb != _vb) || (vb_stride != _vb_stride))
	{
#ifdef DEBUG
		stat.vb++;
#endif
		vb = _vb;
		vb_stride = _vb_stride;
		CHK_DX(HW.GetDevice()->SetStreamSource(0, vb, 0, vb_stride));
	}
}

ICF void CBackend::set_Indices(IDirect3DIndexBuffer9* _ib)
{
	if (ib != _ib)
	{
#ifdef DEBUG
		stat.ib++;
#endif
		ib = _ib;
		CHK_DX(HW.GetDevice()->SetIndices(ib));
	}
}

ICF void CBackend::Apply(u32 countV, u32 PC)
{
	//OPTICK_EVENT("CBackend::Apply");

	stat.calls++;
	stat.verts += countV;
	stat.polys += PC;
	constants.flush();
}

ICF void CBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	Apply(countV, PC);

	CHK_DX(HW.GetDevice()->DrawIndexedPrimitive(PrimitiveType, baseV, startV, countV, startI, PC));
}

ICF void CBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 startV, u32 PC)
{
	stat.calls++;
	stat.verts += 3 * PC;
	stat.polys += PC;
	constants.flush();
	CHK_DX(HW.GetDevice()->DrawPrimitive(PrimitiveType, startV, PC));
}

ICF void CBackend::set_Shader(Shader* S, u32 pass)
{
	set_Element(S->E[0], pass);
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
	set_Format(_geom->dcl._get()->dcl);
	set_Vertices(_geom->vb, _geom->vb_stride);
	set_Indices(_geom->ib);
}

IC void CBackend::set_Scissor(Irect* R)
{
	if (R)
	{
		SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		RECT* clip = (RECT*)R;
		CHK_DX(HW.GetDevice()->SetScissorRect(clip));
	}
	else
	{
		SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
}

IC void CBackend::set_Stencil(u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass,
							  u32 _zfail)
{
	// Simple filter
	if (stencil_enable != _enable)
	{
		stencil_enable = _enable;
		SetRenderState(D3DRS_STENCILENABLE, _enable);
	}
	if (!stencil_enable)
		return;
	if (stencil_func != _func)
	{
		stencil_func = _func;
		SetRenderState(D3DRS_STENCILFUNC, _func);
	}
	if (stencil_ref != _ref)
	{
		stencil_ref = _ref;
		SetRenderState(D3DRS_STENCILREF, _ref);
	}
	if (stencil_mask != _mask)
	{
		stencil_mask = _mask;
		SetRenderState(D3DRS_STENCILMASK, _mask);
	}
	if (stencil_writemask != _writemask)
	{
		stencil_writemask = _writemask;
		SetRenderState(D3DRS_STENCILWRITEMASK, _writemask);
	}
	if (stencil_fail != _fail)
	{
		stencil_fail = _fail;
		SetRenderState(D3DRS_STENCILFAIL, _fail);
	}
	if (stencil_pass != _pass)
	{
		stencil_pass = _pass;
		SetRenderState(D3DRS_STENCILPASS, _pass);
	}
	if (stencil_zfail != _zfail)
	{
		stencil_zfail = _zfail;
		SetRenderState(D3DRS_STENCILZFAIL, _zfail);
	}
}
IC void CBackend::set_ColorWriteEnable(u32 _mask)
{
	if (colorwrite_mask != _mask)
	{
		colorwrite_mask = _mask;
		SetRenderState(D3DRS_COLORWRITEENABLE, _mask);
		SetRenderState(D3DRS_COLORWRITEENABLE1, _mask);
		SetRenderState(D3DRS_COLORWRITEENABLE2, _mask);
		SetRenderState(D3DRS_COLORWRITEENABLE3, _mask);
	}
}
IC void CBackend::set_ZWriteEnable(bool write_state)
{
	if (zwrite != write_state)
	{
		zwrite = write_state;
		SetRenderState(D3DRS_ZWRITEENABLE, write_state);
	}
}
ICF void CBackend::set_CullMode(u32 _mode)
{
	if (cull_mode != _mode)
	{
		cull_mode = _mode;
		SetRenderState(D3DRS_CULLMODE, _mode);
	}
}

ICF void CBackend::set_anisotropy_filtering(int max_anisothropy)
{
	for (u32 i = 0; i < HW.GetCaps().raster.dwStages; i++)
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, max_anisothropy));
}

ENGINE_API extern int psAnisotropic;

ICF void CBackend::enable_anisotropy_filtering()
{
	for (u32 i = 0; i < HW.GetCaps().raster.dwStages; i++)
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, psAnisotropic));
}

ICF void CBackend::disable_anisotropy_filtering()
{
	for (u32 i = 0; i < HW.GetCaps().raster.dwStages; i++)
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1));
}

#endif
