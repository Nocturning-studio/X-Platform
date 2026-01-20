#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(pop)

#include "frustum.h"

void CBackend::OnFrameEnd()
{
#ifndef DEDICATED_SERVER
	for (u32 stage = 0; stage < HW.Caps.raster.dwStages; stage++)
		CHK_DX(HW.pDevice->SetTexture(0, 0));
	CHK_DX(HW.pDevice->SetStreamSource(0, 0, 0, 0));
	CHK_DX(HW.pDevice->SetIndices(0));
	CHK_DX(HW.pDevice->SetVertexShader(0));
	CHK_DX(HW.pDevice->SetPixelShader(0));
	Invalidate();
#endif
}

void CBackend::OnFrameBegin()
{
#ifndef DEDICATED_SERVER
	Memory.mem_fill(&stat, 0, sizeof(stat));
	Vertex.Flush();
	Index.Flush();
	set_Stencil(FALSE);
#endif
}

void CBackend::Invalidate()
{
	//OPTICK_EVENT("CBackend::Invalidate");

	bBlend = FALSE;
	srcBlend = D3DBLEND_ONE;
	dstBlend = D3DBLEND_ZERO;

	pRT[0] = NULL;
	pRT[1] = NULL;
	pRT[2] = NULL;
	pRT[3] = NULL;
	pZB = NULL;

	decl = NULL;
	vb = NULL;
	ib = NULL;
	vb_stride = 0;

	state = NULL;
	ps = NULL;
	vs = NULL;
	ctable = NULL;

	T = NULL;
	M = NULL;
	C = NULL;

	colorwrite_mask = u32(-1);

	for (u32 ps_it = 0; ps_it < 16;)
		textures_ps[ps_it++] = 0;
	for (u32 vs_it = 0; vs_it < 5;)
		textures_vs[vs_it++] = 0;
#ifdef _EDITOR
	for (u32 m_it = 0; m_it < 8;)
		matrices[m_it++] = 0;
#endif
}

void CBackend::SaveRenderState()
{
	for (int i = 0; i < 4; i++)
		HW.pDevice->GetRenderTarget(i, &saved_state.rt[i]);
	HW.pDevice->GetDepthStencilSurface(&saved_state.zb);
	HW.pDevice->GetViewport(&saved_state.viewport);
}

void CBackend::RestoreRenderState()
{
	for (int i = 0; i < 4; i++)
	{
		if (saved_state.rt[i])
		{
			setRenderTarget(saved_state.rt[i], i);
			saved_state.rt[i]->Release();
		}
	}

	if (saved_state.zb)
	{
		setDepthBuffer(saved_state.zb);
		saved_state.zb->Release();
	}

	HW.pDevice->SetViewport(&saved_state.viewport);
}

void CBackend::set_ClipPlanes(u32 _enable, Fplane* _planes /*=NULL */, u32 count /* =0*/)
{
	if (0 == HW.Caps.geometry.dwClipPlanes)
		return;
	if (!_enable)
	{
		SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
		return;
	}

	// Enable and setup planes
	VERIFY(_planes && count);
	if (count > HW.Caps.geometry.dwClipPlanes)
		count = HW.Caps.geometry.dwClipPlanes;

	D3DXMATRIX worldToClipMatrixIT;
	D3DXMatrixInverse(&worldToClipMatrixIT, NULL, (D3DXMATRIX*)&Engine.RenderView.ViewProjection);
	D3DXMatrixTranspose(&worldToClipMatrixIT, &worldToClipMatrixIT);
	for (u32 it = 0; it < count; it++)
	{
		Fplane& P = _planes[it];
		D3DXPLANE planeWorld(-P.n.x, -P.n.y, -P.n.z, -P.d), planeClip;
		D3DXPlaneNormalize(&planeWorld, &planeWorld);
		D3DXPlaneTransform(&planeClip, &planeWorld, &worldToClipMatrixIT);
		CHK_DX(HW.pDevice->SetClipPlane(it, planeClip));
	}

	// Enable them
	u32 e_mask = (1 << count) - 1;
	SetRenderState(D3DRS_CLIPPLANEENABLE, e_mask);
}

#ifndef DEDICATED_SREVER
void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
	if (0 == HW.Caps.geometry.dwClipPlanes)
		return;
	if (!_enable)
	{
		SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
		return;
	}
	VERIFY(_xform && fmask);
	CFrustum F;
	F.CreateFromMatrix(*_xform, fmask);
	set_ClipPlanes(_enable, F.planes, F.p_count);
}

void CBackend::set_Textures(STextureList* _T)
{
	if (T == _T)
		return;
	T = _T;
	u32 _last_ps = 0;
	u32 _last_vs = 0;
	STextureList::iterator _it = _T->begin();
	STextureList::iterator _end = _T->end();
	for (; _it != _end; _it++)
	{
		std::pair<u32, ref_texture>& loader = *_it;
		u32 load_id = loader.first;
		CTexture* load_surf = &*loader.second;
		if (load_id < 256)
		{
			// ordinary pixel surface
			if (load_id > _last_ps)
				_last_ps = load_id;
			if (textures_ps[load_id] != load_surf)
			{
				textures_ps[load_id] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					//OPTICK_EVENT("set_Textures - load_surf");
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
		else
		{
			// d-map or vertex
			u32 load_id_remapped = load_id - 256;
			if (load_id_remapped > _last_vs)
				_last_vs = load_id_remapped;
			if (textures_vs[load_id_remapped] != load_surf)
			{
				textures_vs[load_id_remapped] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					//OPTICK_EVENT("set_Textures - load_surf");
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
	}

	// clear remaining stages (PS)
	for (++_last_ps; _last_ps < 16 && textures_ps[_last_ps]; _last_ps++)
	{
		textures_ps[_last_ps] = 0;
		CHK_DX(HW.pDevice->SetTexture(_last_ps, NULL));
	}
	// clear remaining stages (VS)
	for (++_last_vs; _last_vs < 5 && textures_vs[_last_vs]; _last_vs++)
	{
		textures_vs[_last_vs] = 0;
		CHK_DX(HW.pDevice->SetTexture(_last_vs + 256, NULL));
	}
}
#else

void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
}
void CBackend::set_Textures(STextureList* _T)
{
}

#endif

void CBackend::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (ref_rt& _1)");

	RenderBackend.setRenderTarget(_1->pRT, 0);

	if (_2)
		RenderBackend.setRenderTarget(_2->pRT, 1);
	else
		RenderBackend.setRenderTarget(NULL, 1);

	if (_3)
		RenderBackend.setRenderTarget(_3->pRT, 2);
	else
		RenderBackend.setRenderTarget(NULL, 2);

	if (_4)
		RenderBackend.setRenderTarget(_4->pRT, 3);
	else
		RenderBackend.setRenderTarget(NULL, 3);
}

void CBackend::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (IDirect3DSurface9* _1)");

	//dwWidth = W;
	//dwHeight = H;

	RenderBackend.setRenderTarget(_1, 0);

	if (_2)
		RenderBackend.setRenderTarget(_2, 1);
	else
		RenderBackend.setRenderTarget(NULL, 1);

	if (_3)
		RenderBackend.setRenderTarget(_3, 2);
	else
		RenderBackend.setRenderTarget(NULL, 2);

	if (_4)
		RenderBackend.setRenderTarget(_4, 3);
	else
		RenderBackend.setRenderTarget(NULL, 3);
}

void CBackend::set_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
}

void CBackend::clear_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
	CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_ZBUFFER, 0x0, 1.0f, 0L));
}

void CBackend::set_Blend(BOOL enable, D3DBLEND src, D3DBLEND dest)
{
	// Check if state actually changed to avoid redundant API calls
	if (bBlend != enable || srcBlend != src || dstBlend != dest)
	{
		bBlend = enable;
		srcBlend = src;
		dstBlend = dest;

		SetRenderState(D3DRS_ALPHABLENDENABLE, enable);

		if (enable)
		{
			SetRenderState(D3DRS_SRCBLEND, src);
			SetRenderState(D3DRS_DESTBLEND, dest);

			// Также установим правильные состояния для альфа-тестинга если нужно
			SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		}
	}
}

void CBackend::set_Blend_Alpha()
{
	set_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
}

void CBackend::set_Blend_Add()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CBackend::set_Blend_Multiply()
{
	set_Blend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);
}

void CBackend::set_Blend_Default()
{
	set_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
}

// Также добавим методы для других распространенных blend режимов
void CBackend::set_Blend_Subtract()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
	SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
}

void CBackend::set_Blend_Screen()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR);
}

void CBackend::set_Blend_LightAdd()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CBackend::set_Blend_ColorAdd()
{
	set_Blend(TRUE, D3DBLEND_SRCCOLOR, D3DBLEND_ONE);
}

void CBackend::set_BlendEx(BOOL enable, D3DBLEND src, D3DBLEND dest, D3DBLENDOP op)
{
	if (bBlend != enable || srcBlend != src || dstBlend != dest)
	{
		bBlend = enable;
		srcBlend = src;
		dstBlend = dest;

		SetRenderState(D3DRS_ALPHABLENDENABLE, enable);

		if (enable)
		{
			SetRenderState(D3DRS_SRCBLEND, src);
			SetRenderState(D3DRS_DESTBLEND, dest);
			SetRenderState(D3DRS_BLENDOP, op);

			SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		}
	}
}

// 2D texgen (texture adjustment matrix)
void CBackend::u_compute_texgen_screen(Fmatrix& m_Texgen)
{
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float o_w = (.5f / _w);
	float o_h = (.5f / _h);
	Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 
							 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 
							 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};
	m_Texgen.mul(m_TexelAdjust, RenderBackend.xforms.m_wvp);
}

void CBackend::set_viewport_geometry(u32 w, u32 h, ref_geom geometry, u32& vOffset)
{
	// Constants
	u32 C = color_rgba(0, 0, 0, 255);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, geometry->vb_stride, vOffset);
	pv->set_position(0, (float)h, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p0.x, p1.y);
	pv++;

	pv->set_position(0, 0, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p0.x, p0.y);
	pv++;

	pv->set_position((float)w, (float)h, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p1.x, p1.y);
	pv++;

	pv->set_position((float)w, 0, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p1.x, p0.y);
	pv++;
	RenderBackend.Vertex.Unlock(4, geometry->vb_stride);

	// Set geometry
	RenderBackend.set_Geometry(geometry);
}

void CBackend::set_viewport_geometry(u32 w, u32 h, u32& vOffset)
{
	set_viewport_geometry(w, h, g_viewport, vOffset);
}

void CBackend::set_viewport_geometry(ref_geom geometry, u32& vOffset)
{
	u32 w = Device.dwWidth;
	u32 h = Device.dwHeight;
	set_viewport_geometry(w, h, geometry, vOffset);
}

void CBackend::set_viewport_geometry(u32& vOffset)
{
	u32 w = Device.dwWidth;
	u32 h = Device.dwHeight;
	set_viewport_geometry(w, h, g_viewport, vOffset);
}

void CBackend::render_viewport_geometry(u32 w, u32 h)
{
	u32 vOffset;
	set_viewport_geometry(w, h, g_viewport, vOffset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface()
{
	u32 Offset = 0;
	set_viewport_geometry(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface(const ref_rt& _1, IDirect3DSurface9* zb)
{
	set_Render_Target_Surface(_1);
	set_Depth_Buffer(zb);
	render_viewport_geometry(_1->dwWidth, _1->dwHeight);
}

void CBackend::RenderViewportSurface(u32 w, u32 h, IDirect3DSurface9* _1, IDirect3DSurface9* zb)
{
	set_Render_Target_Surface(w, h, _1);
	set_Depth_Buffer(zb);
	render_viewport_geometry(w, h);
}

void CBackend::RenderViewportSurface(IDirect3DSurface9* _1)
{
	D3DSURFACE_DESC desc;
	HRESULT hr = _1->GetDesc(&desc);

	if (FAILED(hr))
		return;

	set_Render_Target_Surface(desc.Width, desc.Height, _1);
	set_Depth_Buffer(NULL);
	render_viewport_geometry(desc.Width, desc.Height);
}

void CBackend::RenderViewportSurface(u32 w, u32 h, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	set_Depth_Buffer(NULL);
	render_viewport_geometry(w, h);
}

void CBackend::RenderToMipLevel(ref_rt target, u32 mip_level)
{
	 if (!target || !target->valid())
	 {
		 Msg("!CBackend::RenderToMipLevel -  Texture is not present! (Name %s, level %d)", target->cName, mip_level);
		 return;
	 }

	 IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
	 if (!mip_surface)
	 {
		 Msg("!CBackend::RenderToMipLevel -  mip level is not present! (Name %s, level %d)", target->cName, mip_level);
		 return;
	 }

	 u32 width, height;
	 target->get_level_desc(mip_level, width, height);

	 // Сохраняем состояние
	 SaveRenderState();

	 // Рендерим
	 RenderViewportSurface(width, height, mip_surface);

	 // Восстанавливаем состояние
	 RestoreRenderState();

	 mip_surface->Release();
 }

void CBackend::RenderToMipLevel(ref_rt target, u32 mip_level, ShaderElement* shader, u32 pass)
{
	if (!target || !target->valid())
		return;

	IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
	if (!mip_surface)
		return;

	u32 width, height;
	target->get_level_desc(mip_level, width, height);

	// Сохраняем состояние
	SaveRenderState();

	// Устанавливаем шейдер
	set_Element(shader, pass);

	// Рендерим
	RenderViewportSurface(width, height, mip_surface);

	// Восстанавливаем состояние
	RestoreRenderState();

	mip_surface->Release();
}

// Генерация mip-цепочки
void CBackend::GenerateMipChain(ref_rt source, ref_rt mip_chain, ShaderElement* downsample_shader, u32 pass)
{
	if (!source || !mip_chain || !source->valid() || !mip_chain->valid())
		return;

	// Копируем исходное изображение в уровень 0
	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_level0 = mip_chain->get_surface_level(0);

	if (src_surface && dst_level0)
	{
		RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
		RECT dst_rect = {0, 0, 64, 64};
		HW.pDevice->StretchRect(src_surface, &src_rect, dst_level0, &dst_rect, D3DTEXF_LINEAR);
		dst_level0->Release();
	}

	// Генерируем остальные mip-уровни
	for (u32 i = 1; i < mip_chain->get_levels_count(); i++)
	{
		RenderToMipLevel(mip_chain, i, downsample_shader, pass);
	}
}

// Копирование содержимого из одного ref_rt в другой
void CBackend::CopyViewportSurface(ref_rt source, ref_rt destination)
{
	if (!source || !destination || !source->valid() || !destination->valid())
	{
		Msg("! ERROR: CopyViewportSurface - invalid source or destination");
		return;
	}

	// Получаем поверхности
	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if (!src_surface || !dst_surface)
	{
		Msg("! ERROR: CopyViewportSurface - failed to get surfaces");
		return;
	}

	// Определяем области копирования
	RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
	RECT dst_rect = {0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight};

	// Выполняем копирование
	HRESULT hr = HW.pDevice->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, D3DTEXF_LINEAR);

	if (FAILED(hr))
	{
		Msg("! ERROR: CopyViewportSurface - StretchRect failed (0x%08x)", hr);
	}
}

// Версия с указанием фильтра
void CBackend::CopyViewportSurface(ref_rt source, ref_rt destination, D3DTEXTUREFILTERTYPE filter)
{
	if (!source || !destination || !source->valid() || !destination->valid())
		return;

	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if (!src_surface || !dst_surface)
		return;

	RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
	RECT dst_rect = {0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight};

	HW.pDevice->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

// Версия с указанием конкретных областей
void CBackend::CopyViewportSurface(ref_rt source, RECT src_rect, ref_rt destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter = D3DTEXF_LINEAR)
{
	if (!source || !destination || !source->valid() || !destination->valid())
		return;

	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if (!src_surface || !dst_surface)
		return;

	HW.pDevice->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

void CBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination)
{
	if (!source || !destination)
	{
		Msg("! ERROR: CopySurface - invalid source or destination surface");
		return;
	}

	// Получаем описания поверхностей для проверки
	D3DSURFACE_DESC src_desc, dst_desc;
	HRESULT hr1 = source->GetDesc(&src_desc);
	HRESULT hr2 = destination->GetDesc(&dst_desc);

	if (FAILED(hr1) || FAILED(hr2))
	{
		Msg("! ERROR: CopySurface - failed to get surface descriptions");
		return;
	}

	// Определяем области копирования
	RECT src_rect = {0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height};
	RECT dst_rect = {0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height};

	// Выполняем копирование
	HRESULT hr = HW.pDevice->StretchRect(source, &src_rect, destination, &dst_rect, D3DTEXF_LINEAR);

	if (FAILED(hr))
	{
		Msg("! ERROR: CopySurface - StretchRect failed (0x%08x)", hr);
	}
}

// Версия с фильтром
void CBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination, D3DTEXTUREFILTERTYPE filter)
{
	if (!source || !destination)
		return;

	D3DSURFACE_DESC src_desc, dst_desc;
	if (FAILED(source->GetDesc(&src_desc)) || FAILED(destination->GetDesc(&dst_desc)))
		return;

	RECT src_rect = {0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height};
	RECT dst_rect = {0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height};

	HW.pDevice->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}

// Версия с указанием областей
void CBackend::CopySurface(IDirect3DSurface9* source, RECT src_rect, IDirect3DSurface9* destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter = D3DTEXF_NONE)
{
	if (!source || !destination)
		return;

	HW.pDevice->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}

void CBackend::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	CHK_DX(HW.pDevice->Clear(Count, pRects, Flags, Color, Z, Stencil));
}

void CBackend::ClearTexture(const ref_rt& _1, u32 color)
{
	set_Render_Target_Surface(_1, NULL, NULL, NULL);
	Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, u32 color)
{
	set_Render_Target_Surface(_1, _2, NULL, NULL);
	Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, u32 color)
{
	set_Render_Target_Surface(_1, _2, _3, NULL);
	Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4, u32 color)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L);
}