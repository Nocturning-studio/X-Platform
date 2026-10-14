////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
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
