////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <d3dcommon.h>
////////////////////////////////////////////////////////////////////////////////
template <typename Tag, typename T>
class DX9HandlePool
{
  public:
	using Handle = RHI_Handle<Tag>;

	Handle Allocate(T* obj)
	{
		uint32_t idx;
		if(!m_free.empty())
		{
			idx = m_free.top();
			m_free.pop();
			m_data[idx] = obj;
		}
		else
		{
			idx = static_cast<uint32_t>(m_data.size());
			m_data.push_back(obj);
		}
		return Handle{idx};
	}

	T* Get(Handle h) const
	{
		if(!h.IsValid() || h.id >= m_data.size())
			return nullptr;
		return m_data[h.id];
	}

	T* Release(Handle h)
	{
		T* obj = Get(h);
		if(!obj)
			return nullptr;
		m_data[h.id] = nullptr;
		m_free.push(h.id);
		return obj;
	}

	std::vector<T*>& Data() { return m_data; }
	const std::vector<T*>& Data() const { return m_data; }

	void Clear()
	{
		m_data.clear();
		while(!m_free.empty())
			m_free.pop();
	}

  private:
	std::vector<T*> m_data;
	std::stack<uint32_t> m_free;
};

struct DX9Texture
{
	IDirect3DTexture9* tex2D = nullptr;
	IDirect3DCubeTexture9* texCube = nullptr;
	IDirect3DVolumeTexture9* tex3D = nullptr;
	IDirect3DSurface9* surface = nullptr;

	RHI_Format format = RHI_Format::Unknown;
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	RHI_TextureDim dim = RHI_TextureDim::Tex2D;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint32_t mipLevels = 1;
	uint32_t arraySize = 1;
	uint32_t sampleCount = 1;
	uint32_t usage = RHI_TexUsage_None;

	bool isRenderTarget = false;
	bool isDepthStencil = false;
	bool isSampleableDepth = false;

	IDirect3DBaseTexture9* GetBase() const
	{
		if(tex2D)
			return tex2D;
		if(texCube)
			return texCube;
		if(tex3D)
			return tex3D;
		return nullptr;
	}
};

struct DX9Buffer
{
	IDirect3DVertexBuffer9* vb = nullptr;
	IDirect3DIndexBuffer9* ib = nullptr;

	RHI_BufferDesc desc{};
	D3DPOOL pool = D3DPOOL_DEFAULT;
	bool locked = false;
	bool isIndex = false;
};

struct DX9InputLayout
{
	IDirect3DVertexDeclaration9* decl = nullptr;
	RHI_InputLayoutDesc desc;
};

struct DX9Shader
{
	IDirect3DVertexShader9* vs = nullptr;
	IDirect3DPixelShader9* ps = nullptr;
	ID3DBlob* blob = nullptr;
	bool isVertex = false;
};

struct DX9PipelineState
{
	RHI_PipelineStateDesc desc{};
};

struct DX9PipelineLayout
{
	RHI_PipelineLayoutDesc desc{};
};

D3DFORMAT RHIToD3DFormat(RHI_Format fmt);
D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr);
D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f);
size_t GetPixelSize(RHI_Format fmt);

RHI_Format D3DFormatToRHI(D3DFORMAT fmt);

BYTE ToD3DDeclType(RHI_VertexElementType t);
BYTE ToD3DDeclUsage(RHI_VertexElementSemantic s);
D3DPRIMITIVETYPE ToD3DPrimitive(RHI_Topology t);
////////////////////////////////////////////////////////////////////////////////
