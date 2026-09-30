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
	DirectX9 = 0, // Direct3D9Ex
	DirectX12,	  // Direct3D12
};

// =========================================================================
// Форматы данных (Resources)
// =========================================================================
enum class RHI_Format : uint32_t
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
	u32   X = 0;
	u32   Y = 0;
	u32   Width = 0;
	u32   Height = 0;
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

struct RHI_TextureDesc
{
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mipLevels;
	RHI_Format format;
	bool isRenderTarget;
	bool isDepthStencil;
	bool isCubeMap;
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
	uint32_t BackBufferCount = 2;									   // Количество буферов в своп-цепи (1-3)
	uint32_t SyncInterval = 1;										   // 0 - немедленно, 1 - вертикальная синхронизация
	uint32_t FullscreenRefreshHz = 60;								   // Частота обновления (для полноэкранного режима)
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
	OK = 0,     // Устройство работает нормально.
	NeedReset,  // Устройство потеряно, но может быть восстановлено через Reset().
				// D3D9: D3DERR_DEVICENOTRESET.
	Lost,       // Устройство потеряно безвозвратно. Требуется DestroyDevice + CreateDevice.
				// D3D9: D3DERR_DEVICELOST. D3D12: device removed.
};
////////////////////////////////////////////////////////////////////////////////
