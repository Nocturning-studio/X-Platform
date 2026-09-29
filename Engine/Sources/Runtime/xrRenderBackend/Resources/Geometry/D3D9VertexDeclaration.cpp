////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "D3D9VertexDeclaration.h"
#include "xrRenderBackendMacros.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
BYTE ToD3DDeclType(EVertexElementType t)
{
	switch(t)
	{
	case EVertexElementType::Float1:
		return D3DDECLTYPE_FLOAT1;
	case EVertexElementType::Float2:
		return D3DDECLTYPE_FLOAT2;
	case EVertexElementType::Float3:
		return D3DDECLTYPE_FLOAT3;
	case EVertexElementType::Float4:
		return D3DDECLTYPE_FLOAT4;
	case EVertexElementType::UByte4:
		return D3DDECLTYPE_UBYTE4;
	case EVertexElementType::UByte4N:
		return D3DDECLTYPE_UBYTE4N;
	case EVertexElementType::Short2:
		return D3DDECLTYPE_SHORT2;
	case EVertexElementType::Short2N:
		return D3DDECLTYPE_SHORT2N;
	case EVertexElementType::Short4:
		return D3DDECLTYPE_SHORT4;
	case EVertexElementType::Short4N:
		return D3DDECLTYPE_SHORT4N;
		// UInt1..4 не имеют прямого аналога в D3D9 — задел на DX12.
	default:
		return D3DDECLTYPE_UNUSED;
	}
}

BYTE ToD3DDeclUsage(EVertexElementSemantic s)
{
	switch(s)
	{
	case EVertexElementSemantic::Position:
		return D3DDECLUSAGE_POSITION;
	case EVertexElementSemantic::Normal:
		return D3DDECLUSAGE_NORMAL;
	case EVertexElementSemantic::Tangent:
		return D3DDECLUSAGE_TANGENT;
	case EVertexElementSemantic::Binormal:
		return D3DDECLUSAGE_BINORMAL;
	case EVertexElementSemantic::TexCoord:
		return D3DDECLUSAGE_TEXCOORD;
	case EVertexElementSemantic::Color:
		return D3DDECLUSAGE_COLOR;
	case EVertexElementSemantic::BlendWeight:
		return D3DDECLUSAGE_BLENDWEIGHT;
	case EVertexElementSemantic::BlendIndices:
		return D3DDECLUSAGE_BLENDINDICES;
	case EVertexElementSemantic::PositionT:
		return D3DDECLUSAGE_POSITIONT;
	default:
		return D3DDECLUSAGE_POSITION;
	}
}
} // namespace

CD3D9VertexDeclaration::~CD3D9VertexDeclaration()
{
	RELEASE(m_decl);
}

HRESULT CD3D9VertexDeclaration::Create(IDirect3DDevice9Ex* device, const CVertexLayoutDesc& layout)
{
	if(!device || layout.elements.empty())
		return E_INVALIDARG;

	RELEASE(m_decl);
	m_layout = layout;

	xr_vector<D3DVERTEXELEMENT9> elems;
	elems.reserve(layout.elements.size() + 1);

	for(const auto& e : layout.elements)
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

	const HRESULT hr = device->CreateVertexDeclaration(elems.data(), &m_decl);
	if(FAILED(hr))
	{
		Msg("! [D3D9VertexDeclaration] CreateVertexDeclaration failed (hr=0x%08X, elems=%u)", hr, (u32)layout.elements.size());
		return hr;
	}

	return S_OK;
}

void CD3D9VertexDeclaration::Bind(IDirect3DDevice9Ex* device) const
{
	if(device)
		device->SetVertexDeclaration(m_decl);
}

void CD3D9VertexDeclaration::OnDeviceLost()
{
	RELEASE(m_decl);
}

HRESULT CD3D9VertexDeclaration::OnDeviceReset(IDirect3DDevice9Ex* device)
{
	if(m_decl)
		return S_OK;
	return Create(device, m_layout);
}
////////////////////////////////////////////////////////////////////////////////
