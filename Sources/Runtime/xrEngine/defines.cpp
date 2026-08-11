#include "stdafx.h"

#ifdef DEBUG
BOOL bDebug = FALSE;
#endif

// Video
u32 psCurrentVidMode[2] = {1024, 768};
u32 psCurrentBPP = 32;

Flags32 psDeviceFlags = {rsFullscreen | rsDetails | mtPhysics | mtSound | mtNetwork | rsDrawStatic | rsDrawDynamic};

float psVisDistance = 1.0f;

// textures
int psTextureLOD = 0;

// Weapon
u32 psWpnZoomButtonMode = 2;
