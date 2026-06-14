// xrCDB.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#pragma hdrstop

#include "xrCDB.h"
#include <thread>
#include <ppl.h>
#include <ppltasks.h>

using namespace CDB;
using namespace Opcode;

BOOL APIENTRY DllMain(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// Model building
MODEL::MODEL()
#ifdef PROFILE_CRITICAL_SECTIONS
	: cs(MUTEX_PROFILE_ID(MODEL))
#endif // PROFILE_CRITICAL_SECTIONS
{
	tree = 0;
	tris = 0;
	tris_count = 0;
	verts = 0;
	verts_count = 0;
	status = S_INIT;
}
MODEL::~MODEL()
{
	syncronize(); // maybe model still in building
	status = S_INIT;
	xr_delete(tree);
	xr_free(tris);
	tris_count = 0;
	xr_free(verts);
	verts_count = 0;
}

struct BTHREAD_params
{
	MODEL* M;
	fvec3* V;
	int Vcnt;
	TRI* T;
	int Tcnt;
	build_callback* BC;
	void* BCP;
};

void MODEL::build(fvec3* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc, void* bcp)
{
	R_ASSERT(S_INIT == status);
	R_ASSERT((Vcnt >= 4) && (Tcnt >= 2));
	_mm_setcsr(_mm_getcsr() | 0x8000 | 0x0040);
#ifdef _EDITOR
	build_internal(V, Vcnt, T, Tcnt, bc, bcp);
#else

	Msg("* xrCDB: Use dedicated thread for cform building");

	BTHREAD_params* P = xr_new<BTHREAD_params>();
	P->M = this;
	P->V = V;
	P->Vcnt = Vcnt;
	P->T = T;
	P->Tcnt = Tcnt;
	P->BC = bc;
	P->BCP = bcp;

	// Используем PPL
	concurrency::create_task([P]() {
		OPTICK_THREAD("xrCDB Build Thread");
		OPTICK_FRAME("xrCDB Build Thread");

		P->M->cs.Enter();
		P->M->build_internal(P->V, P->Vcnt, P->T, P->Tcnt, P->BC, P->BCP);
		P->M->status = S_READY;
		P->M->cs.Leave();
		Msg("* xrCDB: cform build completed, memory usage: %d K", P->M->memory() / 1024);

		xr_delete(P);
	});

	while (S_INIT == status)
		Sleep(5);
#endif
}

void MODEL::build_internal(fvec3* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc, void* bcp)
{
	// verts
	verts_count = Vcnt;
	verts = xr_alloc<fvec3>(verts_count);
	CopyMemory(verts, V, verts_count * sizeof(fvec3));

	// tris
	tris_count = Tcnt;
	tris = xr_alloc<TRI>(tris_count);
	CopyMemory(tris, T, tris_count * sizeof(TRI));

	// callback
	if (bc)
		bc(verts, Vcnt, tris, Tcnt, bcp);

	// Release data pointers
	status = S_BUILD;

	// Allocate temporary "OPCODE" tris + convert tris to 'pointer' form
	u32* temp_tris = xr_alloc<u32>(tris_count * 3);
	if (0 == temp_tris)
	{
		xr_free(verts);
		xr_free(tris);
		return;
	}
	u32* temp_ptr = temp_tris;
	for (int i = 0; i < tris_count; i++)
	{
		*temp_ptr++ = tris[i].verts[0];
		*temp_ptr++ = tris[i].verts[1];
		*temp_ptr++ = tris[i].verts[2];
	}

	// Build a non quantized no-leaf tree
	OPCODECREATE OPCC;
	OPCC.NbTris = tris_count;
	OPCC.NbVerts = verts_count;
	OPCC.Tris = (unsigned*)temp_tris;
	OPCC.Verts = (Point*)verts;
	OPCC.Rules = SPLIT_COMPLETE | SPLIT_SPLATTERPOINTS | SPLIT_GEOMCENTER;
	OPCC.NoLeaf = true;
	OPCC.Quantized = false;
	// if (Memory.debug_mode) OPCC.KeepOriginal = true;

	tree = xr_new<OPCODE_Model>();
	if (!tree->Build(OPCC))
	{
		xr_free(verts);
		xr_free(tris);
		xr_free(temp_tris);
		return;
	};

	// Free temporary tris
	xr_free(temp_tris);
	return;
}

u32 MODEL::memory()
{
	if (S_BUILD == status)
	{
		Msg("! xrCDB: model still isn't ready");
		return 0;
	}
	u32 V = verts_count * sizeof(fvec3);
	u32 T = tris_count * sizeof(TRI);
	return tree->GetUsedBytes() + V + T + sizeof(*this) + sizeof(*tree);
}

// This is the constructor of a class that has been exported.
// see xrCDB.h for the class definition
COLLIDER::COLLIDER()
{
	ray_mode = 0;
	box_mode = 0;
	frustum_mode = 0;
}

COLLIDER::~COLLIDER()
{
	r_free();
}

RESULT& COLLIDER::r_add()
{
	rd.push_back(RESULT());
	return rd.back();
}

void COLLIDER::r_free()
{
	rd.clear_and_free();
}
