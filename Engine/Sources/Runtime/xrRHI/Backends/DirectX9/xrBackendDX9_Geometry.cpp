////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 15:33:49
// Author: NS_Deathman
// File: xrBackendDX9_Geometry.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
// ============================================================================
// Конвертеры RHI -> D3D9
// ============================================================================

BYTE ToD3DDeclType(RHI_VertexElementType t)
{
	switch(t)
	{
	case RHI_VertexElementType::Float1:
		return D3DDECLTYPE_FLOAT1;
	case RHI_VertexElementType::Float2:
		return D3DDECLTYPE_FLOAT2;
	case RHI_VertexElementType::Float3:
		return D3DDECLTYPE_FLOAT3;
	case RHI_VertexElementType::Float4:
		return D3DDECLTYPE_FLOAT4;
	case RHI_VertexElementType::UByte4:
		return D3DDECLTYPE_UBYTE4;
	case RHI_VertexElementType::UByte4N:
		return D3DDECLTYPE_UBYTE4N;
	case RHI_VertexElementType::Short2:
		return D3DDECLTYPE_SHORT2;
	case RHI_VertexElementType::Short2N:
		return D3DDECLTYPE_SHORT2N;
	case RHI_VertexElementType::Short4:
		return D3DDECLTYPE_SHORT4;
	case RHI_VertexElementType::Short4N:
		return D3DDECLTYPE_SHORT4N;
		// UInt1..4 не имеют аналога в D3D9 — это задел под DX10+.
	default:
		return D3DDECLTYPE_UNUSED;
	}
}

BYTE ToD3DDeclUsage(RHI_VertexElementSemantic s)
{
	switch(s)
	{
	case RHI_VertexElementSemantic::Position:
		return D3DDECLUSAGE_POSITION;
	case RHI_VertexElementSemantic::Normal:
		return D3DDECLUSAGE_NORMAL;
	case RHI_VertexElementSemantic::Tangent:
		return D3DDECLUSAGE_TANGENT;
	case RHI_VertexElementSemantic::Binormal:
		return D3DDECLUSAGE_BINORMAL;
	case RHI_VertexElementSemantic::TexCoord:
		return D3DDECLUSAGE_TEXCOORD;
	case RHI_VertexElementSemantic::Color:
		return D3DDECLUSAGE_COLOR;
	case RHI_VertexElementSemantic::BlendWeight:
		return D3DDECLUSAGE_BLENDWEIGHT;
	case RHI_VertexElementSemantic::BlendIndices:
		return D3DDECLUSAGE_BLENDINDICES;
	case RHI_VertexElementSemantic::PositionT:
		return D3DDECLUSAGE_POSITIONT;
	default:
		return D3DDECLUSAGE_POSITION;
	}
}

D3DPRIMITIVETYPE ToD3DPrimitive(RHI_Topology t)
{
	switch(t)
	{
	case RHI_Topology::PointList:
		return D3DPT_POINTLIST;
	case RHI_Topology::LineList:
		return D3DPT_LINELIST;
	case RHI_Topology::LineStrip:
		return D3DPT_LINESTRIP;
	case RHI_Topology::TriangleList:
		return D3DPT_TRIANGLELIST;
	case RHI_Topology::TriangleStrip:
		return D3DPT_TRIANGLESTRIP;
	case RHI_Topology::TriangleFan:
		return D3DPT_TRIANGLEFAN;
	}
	return D3DPT_TRIANGLELIST;
}

// Количество примитивов по числу вершин/индексов и топологии.
uint32_t PrimitivesFromCount(RHI_Topology t, uint32_t n)
{
	switch(t)
	{
	case RHI_Topology::PointList:
		return n;
	case RHI_Topology::LineList:
		return n / 2;
	case RHI_Topology::LineStrip:
		return (n >= 2) ? n - 1 : 0;
	case RHI_Topology::TriangleList:
		return n / 3;
	case RHI_Topology::TriangleStrip:
		return (n >= 3) ? n - 2 : 0;
	case RHI_Topology::TriangleFan:
		return (n >= 3) ? n - 2 : 0;
	}
	return 0;
}

// Маппинг RHI_BufferUsage -> D3DUSAGE / D3DPOOL.
void ToD3DUsageAndPool(uint32_t rhiUsage, DWORD& outD3DUsage, D3DPOOL& outPool)
{
	outD3DUsage = 0;
	outPool = D3DPOOL_DEFAULT;

	if(rhiUsage & RHI_BufferUsage_Dynamic)
	{
		outD3DUsage |= D3DUSAGE_DYNAMIC;
		outD3DUsage |= D3DUSAGE_WRITEONLY;
	}
}

// Маппинг lock-flags (используем те же биты RHI_BufferUsage_*).
DWORD ToD3DLockFlags(uint32_t lockFlags, uint32_t bufferUsage, bool wholeBuffer)
{
	DWORD f = 0;
	if((lockFlags & RHI_BufferUsage_Dynamic) || (wholeBuffer && (bufferUsage & RHI_BufferUsage_Dynamic)))
		f |= D3DLOCK_DISCARD;
	if(lockFlags & RHI_BufferUsage_CPUReadable)
		f |= D3DLOCK_READONLY;
	return f;
}

} // namespace
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Pool management: buffers
// ============================================================================

RHI_BufferHandle CRenderBackendDX9::AllocBufferHandle(DX9Buffer* buf)
{
	uint32_t idx;
	if(!m_freeBufferIndices.empty())
	{
		idx = m_freeBufferIndices.top();
		m_freeBufferIndices.pop();
		m_buffers[idx] = buf;
	}
	else
	{
		idx = static_cast<uint32_t>(m_buffers.size());
		m_buffers.push_back(buf);
	}
	return RHI_BufferHandle{idx};
}

DX9Buffer* CRenderBackendDX9::GetBuffer(RHI_BufferHandle h) const
{
	if(!h.IsValid() || h.id >= m_buffers.size())
		return nullptr;
	return m_buffers[h.id];
}

void CRenderBackendDX9::FreeBufferHandle(RHI_BufferHandle h)
{
	if(!h.IsValid() || h.id >= m_buffers.size())
		return;

	DX9Buffer* buf = m_buffers[h.id];
	if(!buf)
	{
		Msg("! [DX9] Double free of RHI_BufferHandle(id=%u) detected, ignoring.", h.id);
		return;
	}

	// Если этот буфер привязан к текущему контексту — надо разбиндить
	// перед удалением, иначе D3D9 получит висячий указатель.
	if(!buf->isIndex)
	{
		for(uint32_t i = 0; i < kMaxVertexStreams; ++i)
		{
			if(m_currentVB[i] == buf->vb)
			{
				if(m_pDevice)
					m_pDevice->SetStreamSource(i, nullptr, 0, 0);
				m_currentVB[i] = nullptr;
				m_currentVBStride[i] = 0;
				m_currentVBOffset[i] = 0;
				if(i == 0)
					m_stream0VertexCount = 0;
			}
		}
	}
	else
	{
		if(m_currentIB == buf->ib)
		{
			if(m_pDevice)
				m_pDevice->SetIndices(nullptr);
			m_currentIB = nullptr;
		}
	}

	if(buf->vb)
		buf->vb->Release();
	if(buf->ib)
		buf->ib->Release();

	delete buf;
	m_buffers[h.id] = nullptr;
	m_freeBufferIndices.push(h.id);
}

// ============================================================================
// Pool management: input layouts
// ============================================================================

RHI_InputLayoutHandle CRenderBackendDX9::AllocInputLayoutHandle(DX9InputLayout* lay)
{
	uint32_t idx;
	if(!m_freeInputLayoutIndices.empty())
	{
		idx = m_freeInputLayoutIndices.top();
		m_freeInputLayoutIndices.pop();
		m_inputLayouts[idx] = lay;
	}
	else
	{
		idx = static_cast<uint32_t>(m_inputLayouts.size());
		m_inputLayouts.push_back(lay);
	}
	return RHI_InputLayoutHandle{idx};
}

DX9InputLayout* CRenderBackendDX9::GetInputLayout(RHI_InputLayoutHandle h) const
{
	if(!h.IsValid() || h.id >= m_inputLayouts.size())
		return nullptr;
	return m_inputLayouts[h.id];
}

void CRenderBackendDX9::FreeInputLayoutHandle(RHI_InputLayoutHandle h)
{
	if(!h.IsValid() || h.id >= m_inputLayouts.size())
		return;

	DX9InputLayout* lay = m_inputLayouts[h.id];
	if(!lay)
	{
		Msg("! [DX9] Double free of RHI_InputLayoutHandle(id=%u) detected, ignoring.", h.id);
		return;
	}

	if(m_currentDecl == lay->decl)
	{
		if(m_pDevice)
			m_pDevice->SetVertexDeclaration(nullptr);
		m_currentDecl = nullptr;
	}

	if(lay->decl)
		lay->decl->Release();

	delete lay;
	m_inputLayouts[h.id] = nullptr;
	m_freeInputLayoutIndices.push(h.id);
}

// ============================================================================
// CreateBuffer
// ============================================================================

RHI_BufferHandle CRenderBackendDX9::CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData)
{
	if(!m_pDevice || desc.sizeBytes == 0 || desc.stride == 0)
	{
		Msg("! [DX9] CreateVertexBuffer: invalid args (device=%p, size=%u, stride=%u)",
			m_pDevice, desc.sizeBytes, desc.stride);
		return {};
	}

	DWORD d3dUsage = 0;
	D3DPOOL pool = D3DPOOL_DEFAULT;
	ToD3DUsageAndPool(desc.usage, d3dUsage, pool);

	IDirect3DVertexBuffer9* vb = nullptr;
	const HRESULT hr = m_pDevice->CreateVertexBuffer(desc.sizeBytes, d3dUsage, 0, pool, &vb, nullptr);
	if(FAILED(hr) || !vb)
	{
		Msg("! [DX9] CreateVertexBuffer failed (hr=0x%08x, size=%u, stride=%u, usage=0x%x)", hr, desc.sizeBytes, desc.stride, desc.usage);
		return {};
	}

	if(desc.debugName && desc.debugName[0])
		vb->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);

	DX9Buffer* buf = new DX9Buffer;
	buf->vb = vb;
	buf->desc = desc;
	buf->pool = pool;
	buf->isIndex = false;

	// Загружаем initial data, если дано.
	if(initialData)
	{
		void* p = nullptr;
		const DWORD lf = ToD3DLockFlags(RHI_BufferUsage_Dynamic, desc.usage, true);
		if(SUCCEEDED(vb->Lock(0, 0, &p, lf)) && p)
		{
			memcpy(p, initialData, desc.sizeBytes);
			vb->Unlock();
		}
		else
		{
			Msg("! [DX9] CreateVertexBuffer: initial upload failed");
		}
	}

	return AllocBufferHandle(buf);
}

RHI_BufferHandle CRenderBackendDX9::CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData)
{
	if(!m_pDevice || desc.sizeBytes == 0)
	{
		Msg("! [DX9] CreateIndexBuffer: invalid args (device=%p, size=%u)",
			m_pDevice, desc.sizeBytes);
		return {};
	}

	DWORD d3dUsage = 0;
	D3DPOOL pool = D3DPOOL_DEFAULT;
	ToD3DUsageAndPool(desc.usage, d3dUsage, pool);

	const D3DFORMAT fmt = (desc.indexFormat == RHI_IndexFormat::UInt32) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;

	IDirect3DIndexBuffer9* ib = nullptr;
	const HRESULT hr = m_pDevice->CreateIndexBuffer(desc.sizeBytes, d3dUsage, fmt, pool, &ib, nullptr);
	if(FAILED(hr) || !ib)
	{
		Msg("! [DX9] CreateIndexBuffer failed (hr=0x%08x, size=%u, fmt=%u)", hr, desc.sizeBytes, (uint32_t)desc.indexFormat);
		return {};
	}

	if(desc.debugName && desc.debugName[0])
		ib->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);

	DX9Buffer* buf = new DX9Buffer;
	buf->ib = ib;
	buf->desc = desc;
	buf->pool = pool;
	buf->isIndex = true;

	if(initialData)
	{
		void* p = nullptr;
		const DWORD lf = ToD3DLockFlags(RHI_BufferUsage_Dynamic, desc.usage, true);
		if(SUCCEEDED(ib->Lock(0, 0, &p, lf)) && p)
		{
			memcpy(p, initialData, desc.sizeBytes);
			ib->Unlock();
		}
		else
		{
			Msg("! [DX9] CreateIndexBuffer: initial upload failed");
		}
	}

	return AllocBufferHandle(buf);
}

void CRenderBackendDX9::DestroyBuffer(RHI_BufferHandle handle)
{
	FreeBufferHandle(handle);
}

// ============================================================================
// Lock / Unlock
// ============================================================================

void* CRenderBackendDX9::LockBuffer(RHI_BufferHandle handle, uint32_t offset, uint32_t size, uint32_t flags)
{
	DX9Buffer* buf = GetBuffer(handle);
	if(!buf || !buf->locked == false)
	{
		// D3D9 не поддерживает рекурсивный Lock.
		if(buf && buf->locked)
			Msg("! [DX9] LockBuffer: buffer (id=%u) is already locked", handle.id);
		return nullptr;
	}

	const bool wholeBuffer = (offset == 0 && size == 0);
	const DWORD d3dFlags = ToD3DLockFlags(flags, buf->desc.usage, wholeBuffer);

	void* p = nullptr;
	HRESULT hr = E_FAIL;

	if(buf->isIndex)
		hr = buf->ib->Lock(offset, size, &p, d3dFlags);
	else
		hr = buf->vb->Lock(offset, size, &p, d3dFlags);

	if(FAILED(hr))
	{
		Msg("! [DX9] LockBuffer failed (hr=0x%08x, id=%u, offset=%u, size=%u)", hr, handle.id, offset, size);
		return nullptr;
	}

	buf->locked = true;
	return p;
}

void CRenderBackendDX9::UnlockBuffer(RHI_BufferHandle handle)
{
	DX9Buffer* buf = GetBuffer(handle);
	if(!buf || !buf->locked)
		return;

	if(buf->isIndex)
		buf->ib->Unlock();
	else
		buf->vb->Unlock();

	buf->locked = false;
}

// ============================================================================
// InputLayout
// ============================================================================

RHI_InputLayoutHandle CRenderBackendDX9::CreateInputLayout(const RHI_InputLayoutDesc& desc)
{
	if(!m_pDevice || desc.elements.empty())
	{
		Msg("! [DX9] CreateInputLayout: invalid args (device=%p, elements=%u)", m_pDevice, (uint32_t)desc.elements.size());
		return {};
	}

	xr_vector<D3DVERTEXELEMENT9> elems;
	elems.reserve(desc.elements.size() + 1);

	for(const auto& e : desc.elements)
	{
		D3DVERTEXELEMENT9 de{};
		de.Stream = (WORD)e.stream;
		de.Offset = (WORD)e.offset;
		de.Type = ToD3DDeclType(e.type);
		de.Method = D3DDECLMETHOD_DEFAULT;
		de.Usage = ToD3DDeclUsage(e.semantic);
		de.UsageIndex = (BYTE)e.semanticIndex;
		elems.push_back(de);
	}
	elems.push_back(D3DDECL_END());

	IDirect3DVertexDeclaration9* decl = nullptr;
	const HRESULT hr = m_pDevice->CreateVertexDeclaration(elems.data(), &decl);
	if(FAILED(hr) || !decl)
	{
		Msg("! [DX9] CreateVertexDeclaration failed (hr=0x%08x, elems=%u)", hr, (uint32_t)desc.elements.size());
		return {};
	}

	DX9InputLayout* lay = new DX9InputLayout;
	lay->decl = decl;
	lay->desc = desc;
	return AllocInputLayoutHandle(lay);
}

void CRenderBackendDX9::DestroyInputLayout(RHI_InputLayoutHandle handle)
{
	FreeInputLayoutHandle(handle);
}

// ============================================================================
// Geometry binding
// ============================================================================

void CRenderBackendDX9::SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb,
										uint32_t offset, uint32_t stride)
{
	if(!m_pDevice || slot >= kMaxVertexStreams)
		return;

	IDirect3DVertexBuffer9* nativeVB = nullptr;
	if(vb.IsValid())
	{
		DX9Buffer* buf = GetBuffer(vb);
		if(!buf || buf->isIndex)
		{
			Msg("! [DX9] SetVertexBuffer: invalid handle (id=%u)", vb.id);
			return;
		}
		nativeVB = buf->vb;

		// Если stride не передан — берём из desc буфера. Это удобно,
		// когда вызывающий код не хочет заботиться о согласованности.
		if(stride == 0)
			stride = buf->desc.stride;

		// Обновляем счётчик вершин для stream 0 — нужен DrawIndexed'у.
		if(slot == 0 && stride > 0)
			m_stream0VertexCount = buf->desc.sizeBytes / stride;
	}

	if(m_currentVB[slot] == nativeVB &&
	   m_currentVBStride[slot] == stride &&
	   m_currentVBOffset[slot] == offset)
	{
		return;
	}

	const HRESULT hr = m_pDevice->SetStreamSource(slot, nativeVB, offset, stride);
	if(FAILED(hr))
	{
		Msg("! [DX9] SetStreamSource(%u) failed (hr=0x%08x)", slot, hr);
		return;
	}

	m_currentVB[slot] = nativeVB;
	m_currentVBStride[slot] = stride;
	m_currentVBOffset[slot] = offset;
}

void CRenderBackendDX9::SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt)
{
	if(!m_pDevice)
		return;

	IDirect3DIndexBuffer9* nativeIB = nullptr;
	if(ib.IsValid())
	{
		DX9Buffer* buf = GetBuffer(ib);
		if(!buf || !buf->isIndex)
		{
			Msg("! [DX9] SetIndexBuffer: invalid handle (id=%u)", ib.id);
			return;
		}
		// D3D9 формат зашит в IB при создании. Если пользователь передал
		// другой fmt — предупреждаем, но не отказываем: возможно, это
		// просто невнимательность, а работать всё равно будет по формату IB.
		if(buf->desc.indexFormat != fmt)
		{
			Msg("! [DX9] SetIndexBuffer: fmt mismatch (handle=%u, passed=%u, actual=%u)", ib.id, (uint32_t)fmt, (uint32_t)buf->desc.indexFormat);
		}
		nativeIB = buf->ib;
	}

	if(m_currentIB == nativeIB)
		return;

	const HRESULT hr = m_pDevice->SetIndices(nativeIB);
	if(FAILED(hr))
	{
		Msg("! [DX9] SetIndices failed (hr=0x%08x)", hr);
		return;
	}

	m_currentIB = nativeIB;
}

void CRenderBackendDX9::SetInputLayout(RHI_InputLayoutHandle layout)
{
	if(!m_pDevice)
		return;

	IDirect3DVertexDeclaration9* nativeDecl = nullptr;
	if(layout.IsValid())
	{
		DX9InputLayout* lay = GetInputLayout(layout);
		if(!lay)
		{
			Msg("! [DX9] SetInputLayout: invalid handle (id=%u)", layout.id);
			return;
		}
		nativeDecl = lay->decl;
	}

	if(m_currentDecl == nativeDecl)
		return;

	const HRESULT hr = m_pDevice->SetVertexDeclaration(nativeDecl);
	if(FAILED(hr))
	{
		Msg("! [DX9] SetVertexDeclaration failed (hr=0x%08x)", hr);
		return;
	}

	m_currentDecl = nativeDecl;
}

void CRenderBackendDX9::SetPrimitiveTopology(RHI_Topology topology)
{
	m_currentTopology = topology;
	m_currentD3DTopology = ToD3DPrimitive(topology);
}

// ============================================================================
// Draw
// ============================================================================

void CRenderBackendDX9::Draw(uint32_t vertexCount, uint32_t startVertex)
{
	if(!m_pDevice)
		return;

	const uint32_t primCount = PrimitivesFromCount(m_currentTopology, vertexCount);
	if(primCount == 0)
		return;

	const HRESULT hr = m_pDevice->DrawPrimitive(m_currentD3DTopology, startVertex, primCount);
	if(FAILED(hr))
		Msg("! [DX9] DrawPrimitive failed (hr=0x%08x, topo=%u, start=%u, prim=%u)", hr, (uint32_t)m_currentTopology, startVertex, primCount);
}

void CRenderBackendDX9::DrawIndexed(uint32_t indexCount, uint32_t startIndex, uint32_t baseVertex)
{
	if(!m_pDevice)
		return;

	const uint32_t primCount = PrimitivesFromCount(m_currentTopology, indexCount);
	if(primCount == 0)
		return;

	// D3D9 требует NumVertices — диапазон вершин, которые могут быть
	// затронуты индексами. Мы не анализируем индексы, поэтому передаём
	// полный размер stream 0. Если stream 0 не установлен — рендер
	// отдаст пустоту, что честно отражает ошибку вызывающего кода.
	const HRESULT hr = m_pDevice->DrawIndexedPrimitive(m_currentD3DTopology,
													   static_cast<INT>(baseVertex),
													   0,					 // MinVertexIndex
													   m_stream0VertexCount, // NumVertices
													   startIndex,
													   primCount);

	if(FAILED(hr))
	{
		Msg("! [DX9] DrawIndexedPrimitive failed (hr=0x%08x, topo=%u, base=%u, numV=%u, start=%u, prim=%u)", hr,
			(uint32_t)m_currentTopology,
			baseVertex,
			m_stream0VertexCount,
			startIndex,
			primCount);
	}
}

// ============================================================================
// Invalidate cache
// ============================================================================

void CRenderBackendDX9::InvalidateGeometryCache()
{
	m_currentDecl = nullptr;
	m_currentIB = nullptr;
	m_stream0VertexCount = 0;
	for(uint32_t i = 0; i < kMaxVertexStreams; ++i)
	{
		m_currentVB[i] = nullptr;
		m_currentVBStride[i] = 0;
		m_currentVBOffset[i] = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////
