////////////////////////////////////////////////////////////////////////////////
// Created: 21.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
enum class SceneRenderFlags : u32
{
	None = 0u,

	// Общие на построение сцены и рендеринг
	StaticGeomForward	= 1u << 0,
	StaticGeomDeffered	= 1u << 1,
	DynamicGeomForward	= 1u << 2,
	DynamicGeomDeffered = 1u << 3,
	LODGeometry			= 1u << 4,
	Details				= 1u << 5,
	HUD					= 1u << 6,
	Wallmarks			= 1u << 7,
	Emissive			= 1u << 8,
	AlphaBlend			= 1u << 9,
	Distortion			= 1u << 10,

	// Только для этапа построения сцены
	Lights = 1u << 11,
};

constexpr SceneRenderFlags operator|(SceneRenderFlags a, SceneRenderFlags b) noexcept { return static_cast<SceneRenderFlags>(static_cast<u32>(a) | static_cast<u32>(b)); }
constexpr SceneRenderFlags operator&(SceneRenderFlags a, SceneRenderFlags b) noexcept { return static_cast<SceneRenderFlags>(static_cast<u32>(a) & static_cast<u32>(b)); }
constexpr SceneRenderFlags operator~(SceneRenderFlags a) noexcept { return static_cast<SceneRenderFlags>(~static_cast<u32>(a)); }
inline SceneRenderFlags& operator|=(SceneRenderFlags& a, SceneRenderFlags b) noexcept { return a = a | b; }
inline SceneRenderFlags& operator&=(SceneRenderFlags& a, SceneRenderFlags b) noexcept { return a = a & b; }
constexpr bool has(SceneRenderFlags value, SceneRenderFlags flag) noexcept { return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0u; }

namespace SceneRenderPresets
{
using SRF = SceneRenderFlags;

inline constexpr SRF GatherMainView = SRF::StaticGeomDeffered |
									  SRF::StaticGeomForward |
									  SRF::DynamicGeomDeffered |
									  SRF::DynamicGeomForward |
									  SRF::LODGeometry |
									  SRF::Wallmarks |
									  SRF::Emissive |
									  SRF::AlphaBlend |
									  SRF::Distortion |
									  SRF::HUD |
									  SRF::Lights;

inline constexpr SRF HUDOnly = SRF::HUD;

inline constexpr SRF WallmarksOnly = SRF::Wallmarks;

inline constexpr SRF Opaque = SRF::StaticGeomDeffered |
							  SRF::DynamicGeomDeffered |
							  SRF::LODGeometry;

inline constexpr SRF RenderForwardStage = SRF::StaticGeomForward | 
										  SRF::DynamicGeomForward | 
										  SRF::AlphaBlend |
										  SRF::Emissive;
} // namespace SceneRenderPresets
////////////////////////////////////////////////////////////////////////////////
