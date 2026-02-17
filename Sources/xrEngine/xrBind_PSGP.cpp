#include "stdafx.h"
#include "xrBind_PSGP.h"
#pragma hdrstop

extern xrSkin1W xrSkin1W_SSE;
extern xrSkin2W xrSkin2W_SSE;

void __cdecl xrBind_PSGP(xrDispatchTable* T, DWORD dwFeatures)
{
	T->skin1W = xrSkin1W_SSE;
	T->skin2W = xrSkin2W_SSE;
}
