#pragma once
#include "framework.h"
#include "xrRHI_Internal.h"

RHI_BEGIN
// =========================================================================
// API
// =========================================================================
enum class BackendType : u32
{
	DirectX9 = 0, // Direct3D9Ex
	DirectX11,	   // Direct3D11 (для будущего)
	DirectX12,	   // Direct3D12
	Vulkan,		   // Vulkan
	OpenGL,		   // OpenGL / OpenGL ES
};

// =========================================================================
// Форматы данных (Resources)
// =========================================================================
enum class RHI_Format : u32
{
	Unknown = 0,
	NULLRT,

	// --- Color Formats ---
	RGBA8_UNORM, // D3DFMT_A8R8G8B8
	A8_UNORM,	 // D3DFMT_A8 (или L8 в зависимости от контекста)
	R8_UNORM,	 // D3DFMT_L8

	RGBA16_FLOAT, // D3DFMT_A16B16G16R16F
	RG16_FLOAT,	  // D3DFMT_G16R16F
	R16_FLOAT,	  // D3DFMT_R16F

	// --- Depth/Stencil Formats ---
	D16_UNORM,		   // D3DFMT_D16
	D24_UNORM_S8_UINT, // D3DFMT_D24S8
	D32_FLOAT,		   // D3DFMT_D32F_LOCKABLE

	// Legacy / Specific D3D9 Formats
	D15S1,		  // D3DFMT_D15S1
	D24X8,		  // D3DFMT_D24X8
	D32_LOCKABLE, // D3DFMT_D32 (Integer)

	// Vendor Specific
	D24S8_Shadow, // INTZ
	D16_Shadow,	  // DF16
	D24X4S4,	  // D3DFMT_D24X4S4

	// --- Index Buffers ---
	Index16,
	Index32
};

// =========================================================================
// Топология (Input Assembly)
// =========================================================================
enum class RHI_Topology : u32
{
	PointList = 1, // D3DPT_POINTLIST
	LineList,	   // D3DPT_LINELIST
	LineStrip,	   // D3DPT_LINESTRIP
	TriangleList,  // D3DPT_TRIANGLELIST
	TriangleStrip, // D3DPT_TRIANGLESTRIP
	TriangleFan	   // D3DPT_TRIANGLEFAN
};

// =========================================================================
// Растеризация (Rasterizer State)
// =========================================================================
enum class RHI_CullMode : u32
{
	None = 1,		 // D3DCULL_NONE
	Clockwise,		 // D3DCULL_CW
	CounterClockwise // D3DCULL_CCW
};

enum class RHI_FillMode : u32
{
	Point = 1, // D3DFILL_POINT
	Wireframe, // D3DFILL_WIREFRAME
	Solid	   // D3DFILL_SOLID
};

// =========================================================================
// Тест глубины и трафарета (Depth Stencil State)
// =========================================================================
enum class RHI_CmpFunc : u32
{
	Never = 1,	  // D3DCMP_NEVER
	Less,		  // D3DCMP_LESS
	Equal,		  // D3DCMP_EQUAL
	LessEqual,	  // D3DCMP_LESSEQUAL
	Greater,	  // D3DCMP_GREATER
	NotEqual,	  // D3DCMP_NOTEQUAL
	GreaterEqual, // D3DCMP_GREATEREQUAL
	Always		  // D3DCMP_ALWAYS
};

enum class RHI_StencilOp : u32
{
	Keep = 1, // D3DSTENCILOP_KEEP
	Zero,	  // D3DSTENCILOP_ZERO
	Replace,  // D3DSTENCILOP_REPLACE
	IncrSat,  // D3DSTENCILOP_INCRSAT
	DecrSat,  // D3DSTENCILOP_DECRSAT
	Invert,	  // D3DSTENCILOP_INVERT
	Incr,	  // D3DSTENCILOP_INCR
	Decr	  // D3DSTENCILOP_DECR
};

// =========================================================================
// Смешивание цветов (Blend State)
// =========================================================================
enum class RHI_Blend : u32
{
	Zero = 1,	  // D3DBLEND_ZERO
	One,		  // D3DBLEND_ONE
	SrcColor,	  // D3DBLEND_SRCCOLOR
	InvSrcColor,  // D3DBLEND_INVSRCCOLOR
	SrcAlpha,	  // D3DBLEND_SRCALPHA
	InvSrcAlpha,  // D3DBLEND_INVSRCALPHA
	DestAlpha,	  // D3DBLEND_DESTALPHA
	InvDestAlpha, // D3DBLEND_INVDESTALPHA
	DestColor,	  // D3DBLEND_DESTCOLOR
	InvDestColor, // D3DBLEND_INVDESTCOLOR
	SrcAlphaSat	  // D3DBLEND_SRCALPHASAT
};

enum class RHI_BlendOp : u32
{
	Add = 1,	 // D3DBLENDOP_ADD
	Subtract,	 // D3DBLENDOP_SUBTRACT
	RevSubtract, // D3DBLENDOP_REVSUBTRACT
	Min,		 // D3DBLENDOP_MIN
	Max			 // D3DBLENDOP_MAX
};

// =========================================================================
// Текстурирование и Семплеры (Samplers)
// =========================================================================
enum class RHI_TextureAddress : u32
{
	Wrap = 1,  // D3DTADDRESS_WRAP
	Mirror,	   // D3DTADDRESS_MIRROR
	Clamp,	   // D3DTADDRESS_CLAMP
	Border,	   // D3DTADDRESS_BORDER
	MirrorOnce // D3DTADDRESS_MIRRORONCE
};

enum class RHI_Filter : u32
{
	None = 0,	   // D3DTEXF_NONE
	Point,		   // D3DTEXF_POINT
	Linear,		   // D3DTEXF_LINEAR
	Anisotropic,   // D3DTEXF_ANISOTROPIC
	PyramidalQuad, // D3DTEXF_PYRAMIDALQUAD
	GaussianQuad   // D3DTEXF_GAUSSIANQUAD
};

// =========================================================================
// Вспомогательные флаги
// =========================================================================
enum RHI_ClearFlags : u32
{
	RHI_CLEAR_TARGET = 0x00000001L,	 // D3DCLEAR_TARGET
	RHI_CLEAR_ZBUFFER = 0x00000002L, // D3DCLEAR_ZBUFFER
	RHI_CLEAR_STENCIL = 0x00000004L	 // D3DCLEAR_STENCIL
};

// Описание вьюпорта
struct RHI_Viewport
{
	u32 X;
	u32 Y;
	u32 Width;
	u32 Height;
	float MinZ;
	float MaxZ;
};

// Описание прямоугольника (Scissor Rect)
struct RHI_Rect
{
	s32 left;
	s32 top;
	s32 right;
	s32 bottom;
};

struct TextureDesc
{
	u32 width;
	u32 height;
	u32 depth;
	u32 mipLevels;
	RHI_Format format;
	bool isRenderTarget;
	bool isDepthStencil;
	bool isCubeMap;
};

struct RHITextureImpl;
typedef RHITextureImpl* RHITexture;

struct SamplerDesc
{
	RHI_Filter minFilter;
	RHI_Filter magFilter;
	RHI_Filter mipFilter;
	RHI_TextureAddress addressU;
	RHI_TextureAddress addressV;
	RHI_TextureAddress addressW;
	float mipLODBias;
	u32 maxAnisotropy;
	float4 borderColor;
};

struct RHISamplerImpl;
typedef RHISamplerImpl* RHISampler;

RHI_END
