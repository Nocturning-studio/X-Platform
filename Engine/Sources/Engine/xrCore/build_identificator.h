////////////////////////////////////////////////////////////////////////////////
//  X-Platform / Nocturning Studio

//  SPDX-FileCopyrightText: $year$ Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Некоммерческое использование, копирование и изменение разрешены.
//  Указание авторства не требуется; сохранение этой плашки приветствуется
//  и требуется лицензией при распространении. Коммерческое использование
//  без отдельного разрешения запрещено.
//  Лицензия: https://polyformproject.org/licenses/noncommercial/1.0.0
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
////////////////////////////////////////////////////////////////////////////////
struct xrBuildInfo
{
	u32 ID;
	LPCSTR Date;
	LPCSTR Type;
};
extern XRCORE_API xrBuildInfo GlobalBuildInfo;
////////////////////////////////////////////////////////////////////////////////
XRCORE_API xrBuildInfo ComputeBuildIdentificator();
XRCORE_API void PrintBuildIdentificator(xrBuildInfo BuildID);
XRCORE_API void InitializeGlobalBuildID();
////////////////////////////////////////////////////////////////////////////////
