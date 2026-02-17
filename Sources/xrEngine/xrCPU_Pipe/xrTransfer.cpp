#include "stdafx.h"

#define GeomBytes 24 // pos+norm

void __stdcall xrTransfer_x86(LPVOID vDest, LPVOID vSrc, u32 vCount, u32 vStride, LPWORD iDest, LPWORD iSrc, u32 iCount,
							  u32 iOffset, float4x4* transform)
{
	// Transfer vertices
	if (transform)
	{
		LPBYTE sit = LPBYTE(vSrc);
		LPBYTE send = sit + vCount * vStride;
		LPBYTE dit = LPBYTE(vDest);
		DWORD remain = vStride - GeomBytes; // 2fvector of 3 floats

		switch (remain)
		{
		case 8: // 32 byte vertex	(pos(12)+norm(12)+uv1(8))
			for (; sit != send; sit += vStride, dit += vStride)
			{
				float3* sP = (float3*)sit;
				float3* dP = (float3*)dit;
				float3* sN = (float3*)(sit + 3 * 4);
				float3* dN = (float3*)(dit + 3 * 4);
				transform->transform_tiny(*dP, *sP);
				transform->transform_dir(*dN, *sN);
				CopyMemory(dit + GeomBytes, sit + GeomBytes, 8);
			}
			break;
		case 16: // 40 byte vertex	(pos(12)+norm(12)+uv1(8)+uv2(8))
			for (; sit != send; sit += vStride, dit += vStride)
			{
				float3* sP = (float3*)sit;
				float3* dP = (float3*)dit;
				float3* sN = (float3*)(sit + 3 * 4);
				float3* dN = (float3*)(dit + 3 * 4);
				transform->transform_tiny(*dP, *sP);
				transform->transform_dir(*dN, *sN);
				CopyMemory(dit + GeomBytes, sit + GeomBytes, 16);
			}
			break;
		default: // any size
			for (; sit != send; sit += vStride, dit += vStride)
			{
				float3* sP = (float3*)sit;
				float3* dP = (float3*)dit;
				float3* sN = (float3*)(sit + 3 * 4);
				float3* dN = (float3*)(dit + 3 * 4);
				transform->transform_tiny(*dP, *sP);
				transform->transform_dir(*dN, *sN);
				CopyMemory(dit + GeomBytes, sit + GeomBytes, remain);
			}
			break;
		}
	}
	else
	{
		CopyMemory(vDest, vSrc, vCount * vStride);
	}

	// Transfer indices (in 32bit lines)
	VERIFY(iOffset < 65535);
	{
		DWORD item = (iOffset << 16) | iOffset;
		DWORD count = iCount / 2;
		LPDWORD sit = LPDWORD(iSrc);
		LPDWORD send = sit + count;
		LPDWORD dit = LPDWORD(iDest);
		for (; sit != send; dit++, sit++)
			*dit = *sit + item;
		if (iCount & 1)
			iDest[iCount - 1] = iSrc[iCount - 1] + u16(iOffset);
	}
}
