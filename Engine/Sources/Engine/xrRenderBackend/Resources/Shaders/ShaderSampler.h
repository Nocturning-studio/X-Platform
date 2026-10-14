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
enum class ESamplerFilter : uint8_t
{
	Point,
	Linear,
	Anisotropic,
};

enum class ESamplerAddress : uint8_t
{
	Wrap,
	Clamp,
	Mirror,
	Border,
	MirrorOnce,
};

struct CSamplerDesc
{
	ESamplerFilter minFilter = ESamplerFilter::Linear;
	ESamplerFilter magFilter = ESamplerFilter::Linear;
	ESamplerFilter mipFilter = ESamplerFilter::Linear;
	ESamplerAddress addressU = ESamplerAddress::Wrap;
	ESamplerAddress addressV = ESamplerAddress::Wrap;
	ESamplerAddress addressW = ESamplerAddress::Wrap;
	uint32_t maxAnisotropy = 1;
	uint32_t borderColor = 0;
	float mipLodBias = 0.0f;

	static CSamplerDesc Default() { return {}; }

	static CSamplerDesc Point()
	{
		CSamplerDesc d;
		d.minFilter = d.magFilter = d.mipFilter = ESamplerFilter::Point;
		return d;
	}

	static CSamplerDesc Linear()
	{
		CSamplerDesc d;
		return d;
	}

	static CSamplerDesc Anisotropic(uint32_t maxAniso = 16)
	{
		CSamplerDesc d;
		d.minFilter = ESamplerFilter::Anisotropic;
		d.magFilter = ESamplerFilter::Anisotropic;
		d.mipFilter = ESamplerFilter::Linear;
		d.maxAnisotropy = maxAniso;
		return d;
	}

	static CSamplerDesc Clamp()
	{
		CSamplerDesc d;
		d.addressU = d.addressV = d.addressW = ESamplerAddress::Clamp;
		return d;
	}
};

void ApplySamplerDesc(IDirect3DDevice9* device, uint32_t stage, const CSamplerDesc& desc);
////////////////////////////////////////////////////////////////////////////////
