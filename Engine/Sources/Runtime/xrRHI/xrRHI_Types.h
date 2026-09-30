////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
// =========================================================================
// API
// =========================================================================
enum class RHI_BackendType : uint32_t
{
	DirectX9Ex = 0,
	DirectX12,
};

// =========================================================================
// Форматы данных (Resources)
// =========================================================================
enum class RHI_Format : uint32_t
{
	Unknown = 0,
	NULLRT,

	// --- Color: 8-bit ---
	RGBA8_UNORM,	  // D3DFMT_A8R8G8B8
	RGBA8_UNORM_SRGB, // D3DFMT_A8R8G8B8 + D3DSAMP_SRGBTEXTURE (в D3D12 — отдельный формат)
	A8_UNORM,		  // D3DFMT_A8
	R8_UNORM,		  // D3DFMT_L8

	// --- Color: 10/16-bit packed ---
	R10G10B10A2_UNORM, // D3DFMT_A2B10G10R10

	// --- Color: half-float ---
	RGBA16_FLOAT, // D3DFMT_A16B16G16R16F
	RG16_FLOAT,	  // D3DFMT_G16R16F
	R16_FLOAT,	  // D3DFMT_R16F

	// --- Color: 16-bit UNORM ---
	R16G16B16A16_UNORM, // D3DFMT_A16B16G16R16

	// --- Color: 32-bit float ---
	R32G32B32A32_FLOAT, // D3DFMT_A32B32G32R32F
	R32_FLOAT,			// D3DFMT_R32F

	// --- Depth/Stencil ---
	D16_UNORM,		   // D3DFMT_D16
	D24_UNORM_S8_UINT, // D3DFMT_D24S8
	D32_FLOAT,		   // D3DFMT_D32F_LOCKABLE

	// --- Legacy D3D9-specific ---
	D15S1,		  // D3DFMT_D15S1
	D24X8,		  // D3DFMT_D24X8
	D32_LOCKABLE, // D3DFMT_D32

	// --- Vendor-specific (shadow maps) ---
	D24S8_Shadow, // INTZ
	D16_Shadow,	  // DF16
	D24X4S4,	  // D3DFMT_D24X4S4

	// --- Compressed ---
	BC1_UNORM, // D3DFMT_DXT1
	BC3_UNORM, // D3DFMT_DXT5
	BC5_UNORM, // ATI2 / 3Dc

	// --- Index buffers ---
	Index16,
	Index32,
};

// =========================================================================
// Топология (Input Assembly)
// =========================================================================
enum class RHI_Topology : uint32_t
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
enum class RHI_CullMode : uint32_t
{
	None = 1,		 // D3DCULL_NONE
	Clockwise,		 // D3DCULL_CW
	CounterClockwise // D3DCULL_CCW
};

enum class RHI_FillMode : uint32_t
{
	Point = 1, // D3DFILL_POINT
	Wireframe, // D3DFILL_WIREFRAME
	Solid	   // D3DFILL_SOLID
};

// =========================================================================
// Тест глубины и трафарета (Depth Stencil State)
// =========================================================================
enum class RHI_CmpFunc : uint32_t
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

enum class RHI_StencilOp : uint32_t
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
enum class RHI_Blend : uint32_t
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

enum class RHI_BlendOp : uint32_t
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
enum class RHI_TextureAddress : uint32_t
{
	Wrap = 1,  // D3DTADDRESS_WRAP
	Mirror,	   // D3DTADDRESS_MIRROR
	Clamp,	   // D3DTADDRESS_CLAMP
	Border,	   // D3DTADDRESS_BORDER
	MirrorOnce // D3DTADDRESS_MIRRORONCE
};

enum class RHI_Filter : uint32_t
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
enum RHI_ClearFlags : uint32_t
{
	RHI_CLEAR_TARGET = 0x00000001L,	 // D3DCLEAR_TARGET
	RHI_CLEAR_ZBUFFER = 0x00000002L, // D3DCLEAR_ZBUFFER
	RHI_CLEAR_STENCIL = 0x00000004L	 // D3DCLEAR_STENCIL
};

// Описание вьюпорта
struct RHI_Viewport
{
	u32 X = 0;
	u32 Y = 0;
	u32 Width = 0;
	u32 Height = 0;
	float MinZ = 0.0f;
	float MaxZ = 1.0f;

	constexpr bool operator==(const RHI_Viewport& o) const noexcept
	{
		return X == o.X && Y == o.Y && Width == o.Width && Height == o.Height && MinZ == o.MinZ && MaxZ == o.MaxZ;
	}
	constexpr bool operator!=(const RHI_Viewport& o) const noexcept { return !(*this == o); }
};

// Описание прямоугольника (Scissor Rect)
struct RHI_Rect
{
	s32 left = 0;
	s32 top = 0;
	s32 right = 0;
	s32 bottom = 0;

	constexpr bool operator==(const RHI_Rect& o) const noexcept
	{
		return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
	}
	constexpr bool operator!=(const RHI_Rect& o) const noexcept { return !(*this == o); }
};

enum class RHI_TextureDim : uint8_t
{
	Tex1D,
	Tex2D,
	Tex3D,
	Cube,
};

enum RHI_TextureUsage : uint32_t
{
	RHI_TexUsage_None = 0,
	RHI_TexUsage_ShaderResource = 1u << 0,
	RHI_TexUsage_RenderTarget = 1u << 1,
	RHI_TexUsage_DepthStencil = 1u << 2,
	RHI_TexUsage_Unordered = 1u << 3,
	RHI_TexUsage_CPUReadable = 1u << 4,
	RHI_TexUsage_CPUWritable = 1u << 5,
	RHI_TexUsage_GenerateMips = 1u << 6,
};

struct RHI_TextureDesc
{
	RHI_TextureDim dim = RHI_TextureDim::Tex2D;
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t depth = 1;
	uint32_t mipLevels = 1; // 0 = full chain
	uint32_t arraySize = 1;
	uint32_t sampleCount = 1; // 1 = no MSAA (D3D9: не поддерживается)
	uint32_t sampleQuality = 0;
	RHI_Format format = RHI_Format::RGBA8_UNORM;
	uint32_t usage = RHI_TexUsage_ShaderResource;

	const char* debugName = nullptr;

	bool IsRenderTarget() const { return (usage & RHI_TexUsage_RenderTarget) != 0; }
	bool IsDepthStencil() const { return (usage & RHI_TexUsage_DepthStencil) != 0; }
	bool IsCubeMap() const { return dim == RHI_TextureDim::Cube; }

	static RHI_TextureDesc RenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips = 1)
	{
		RHI_TextureDesc d;
		d.width = w;
		d.height = h;
		d.mipLevels = mips;
		d.format = fmt;
		d.usage = RHI_TexUsage_RenderTarget | RHI_TexUsage_ShaderResource;
		return d;
	}

	static RHI_TextureDesc DepthStencil(uint32_t w, uint32_t h, RHI_Format fmt = RHI_Format::D24_UNORM_S8_UINT)
	{
		RHI_TextureDesc d;
		d.width = w;
		d.height = h;
		d.format = fmt;
		d.usage = RHI_TexUsage_DepthStencil;
		return d;
	}

	static RHI_TextureDesc Color2D(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips = 1)
	{
		RHI_TextureDesc d;
		d.width = w;
		d.height = h;
		d.mipLevels = mips;
		d.format = fmt;
		d.usage = RHI_TexUsage_ShaderResource;
		return d;
	}

	static RHI_TextureDesc Cube(uint32_t edge, RHI_Format fmt, uint32_t mips = 1)
	{
		RHI_TextureDesc d;
		d.dim = RHI_TextureDim::Cube;
		d.width = edge;
		d.height = edge;
		d.mipLevels = mips;
		d.format = fmt;
		d.usage = RHI_TexUsage_ShaderResource;
		return d;
	}
};

// =========================================================================
// Параметры SwapChain (Presentation Params)
// =========================================================================
enum class RHI_SwapEffect : uint32_t
{
	Discard = 0, // D3DSWAPEFFECT_DISCARD / VK_SWAPCHAIN_CREATE_MODE_*
	Flip,		 // Для DXGI / Vulkan (Flip Model)
};

struct RHI_PresentationParams
{
	uint32_t BackBufferWidth = 0;
	uint32_t BackBufferHeight = 0;
	bool Windowed = true;
	RHI_Format BackBufferFormat = RHI_Format::RGBA8_UNORM;		   // Базовый формат бэкбуфера
	RHI_Format DepthStencilFormat = RHI_Format::D24_UNORM_S8_UINT; // Формат для авто-буфера глубины
	uint32_t BackBufferCount = 2;								   // Количество буферов в своп-цепи (1-3)
	uint32_t SyncInterval = 1;									   // 0 - немедленно, 1 - вертикальная синхронизация
	uint32_t FullscreenRefreshHz = 60;							   // Частота обновления (для полноэкранного режима)
	RHI_SwapEffect SwapEffect = RHI_SwapEffect::Discard;
	uint32_t MultisampleCount = 1; // Количество сэмплов (1 = MSAA выключен)
	uint32_t MultisampleQuality = 0;
	bool EnableAutoDepthStencil = true; // Создавать ли автоматический Depth/Stencil буфер
};

// =========================================================================
// Состояние устройства
// =========================================================================
//
// D3D9: TestCooperativeLevel() возвращает три разных кода, что и даёт три
//       состояния. D3D9Ex: тот же принцип (плюс D3DERR_DEVICEREMOVED).
// D3D12: OK | Lost — device loss определяется через GetDeviceRemovedReason()
//       и HRESULT от Present/ExecuteCommandLists. Промежуточного состояния
//       "нужен reset" там нет - сразу Lost с последующим полным пересозданием.
//
// =========================================================================
enum class RHI_DeviceStatus : u32
{
	OK = 0,	   // Устройство работает нормально.
	NeedReset, // Устройство потеряно, но может быть восстановлено через Reset().
			   // D3D9: D3DERR_DEVICENOTRESET.
	Lost,	   // Устройство потеряно безвозвратно. Требуется DestroyDevice + CreateDevice.
		  // D3D9: D3DERR_DEVICELOST. D3D12: device removed.
};
////////////////////////////////////////////////////////////////////////////////
