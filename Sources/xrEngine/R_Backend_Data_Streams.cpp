//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
//////////////////////////////////////////////////////////////////////
#include "ResourceManager.h"
#include "R_Backend_Data_Streams.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int rsDVB_Size = 1024 * 8;
int rsDIB_Size = 1024 * 8;
//////////////////////////////////////////////////////////////////////
void VertexStream::Create()
{
	OPTICK_EVENT("VertexStream::Create");

	Device.Resources->Evict();

	mSize = rsDVB_Size * 1024;
	R_CHK(HW.pDevice->CreateVertexBuffer(mSize, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &pVB, NULL));
	R_ASSERT(pVB);

	mPosition = 0;
	mDiscardID = 0;

	Msg("* Dynamic vertex buffer created: %dK", mSize / 1024);
}

void VertexStream::Destroy()
{
	OPTICK_EVENT("VertexStream::Destroy");

	_RELEASE(pVB);
	_clear();
}

void* VertexStream::Lock(u32 vl_Count, u32 Stride, u32& vOffset)
{
#ifdef DEBUG
	VERIFY(0 == dbg_lock);
	dbg_lock++;
#endif

	// Ensure there is enough space in the VB for this data
	u32 bytes_need = vl_Count * Stride;
	VERIFY((bytes_need <= mSize) && vl_Count);

	// Vertex-local info
	u32 vl_mSize = mSize / Stride;
	u32 vl_mPosition = mPosition / Stride + 1;

	// Check if there is need to flush and perform lock
	BYTE* pData = 0;
	if ((vl_Count + vl_mPosition) >= vl_mSize)
	{
		// FLUSH-LOCK
		mPosition = 0;
		vOffset = 0;
		mDiscardID++;

		pVB->Lock(mPosition, bytes_need, (void**)&pData, LOCKFLAGS_FLUSH);
	}
	else
	{
		// APPEND-LOCK
		mPosition = vl_mPosition * Stride;
		vOffset = vl_mPosition;

		pVB->Lock(mPosition, bytes_need, (void**)&pData, LOCKFLAGS_APPEND);
	}
	VERIFY(pData);

	return LPVOID(pData);
}

void VertexStream::Unlock(u32 Count, u32 Stride)
{
#ifdef DEBUG
	VERIFY(1 == dbg_lock);
	dbg_lock--;
#endif
	mPosition += Count * Stride;

	VERIFY(pVB);
	pVB->Unlock();
}

void VertexStream::reset_begin()
{
	OPTICK_EVENT("VertexStream::reset_begin");

	old_pVB = pVB;
	Destroy();
}
void VertexStream::reset_end()
{
	OPTICK_EVENT("VertexStream::reset_end");

	Create();
}

//////////////////////////////////////////////////////////////////////////
void IndexStream::Create()
{
	OPTICK_EVENT("IndexStream::Create");

	Device.Resources->Evict();

	mSize = rsDIB_Size * 1024;
	R_CHK(HW.pDevice->CreateIndexBuffer(mSize, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pIB, NULL));
	R_ASSERT(pIB);

	mPosition = 0;
	mDiscardID = 0;

	Msg("* Dynamic index buffer created: %dK\n", mSize / 1024);
}

void IndexStream::Destroy()
{
	OPTICK_EVENT("IndexStream::Destroy");

	_RELEASE(pIB);
	_clear();
}

u16* IndexStream::Lock(u32 Count, u32& vOffset)
{
	vOffset = 0;
	BYTE* pLockedData = 0;

	// Ensure there is enough space in the VB for this data
	R_ASSERT((2 * Count <= mSize) && Count);

	// If either user forced us to flush,
	// or there is not enough space for the index data,
	// then flush the buffer contents
	u32 dwFlags = LOCKFLAGS_APPEND;
	if (2 * (Count + mPosition) >= mSize)
	{
		mPosition = 0;			   // clear position
		dwFlags = LOCKFLAGS_FLUSH; // discard it's contens
		mDiscardID++;
	}
	pIB->Lock(mPosition * 2, Count * 2, (void**)&pLockedData, dwFlags);
	VERIFY(pLockedData);

	vOffset = mPosition;

	return LPWORD(pLockedData);
}

void IndexStream::Unlock(u32 RealCount)
{
	mPosition += RealCount;
	VERIFY(pIB);
	pIB->Unlock();
}

void IndexStream::reset_begin()
{
	OPTICK_EVENT("IndexStream::reset_begin");

	old_pIB = pIB;
	Destroy();
}
void IndexStream::reset_end()
{
	OPTICK_EVENT("IndexStream::reset_end");

	Create();
	// old_pIB				= NULL;
}
//////////////////////////////////////////////////////////////////////
