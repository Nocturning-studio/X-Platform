#include "stdafx.h"
#include "mmintrin.h"

void __stdcall xrSlerp_x86(_quaternion<float>* D, _quaternion<float>* Q1, _quaternion<float>* Q2, float t)
{
	D->slerp(*Q1, *Q2, t);
}
