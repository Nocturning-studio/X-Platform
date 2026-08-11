#pragma once

// Forward references
struct ENGINE_API vertRender;
struct ENGINE_API vertBoned1W;
struct ENGINE_API vertBoned2W;
class ENGINE_API CBoneInstance;

// Skinning processor specific functions
typedef void __stdcall xrSkin1W(vertRender* D, vertBoned1W* S, u32 vCount, CBoneInstance* Bones);
typedef void __stdcall xrSkin2W(vertRender* D, vertBoned2W* S, u32 vCount, CBoneInstance* Bones);

#pragma pack(push, 8)
struct xrDispatchTable
{
	xrSkin1W* skin1W;
	xrSkin2W* skin2W;
};
#pragma pack(pop)

// Binder
// NOTE: Engine calls function named "_xrBindPSGP"
typedef void __cdecl xrBinder(xrDispatchTable* T, u32 dwFeatures);

extern xrDispatchTable PSGP;
