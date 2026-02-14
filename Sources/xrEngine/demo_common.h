/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "stdafx.h"
/////////////////////////////////////////////////////////////////
#define MAX_FRAMES_COUNT 1024
/////////////////////////////////////////////////////////////////
// Данные, которые привязываются к каждому ключевому кадру демо
struct FrameData
{
	// Critical params for camera transform matrix
	float3 HPB;
	float3 Position;

	// Jump/Spline camera moving parameter
	u32 InterpolationType;

	// Postprocess
	float3 DOF;

	// Overlays
	bool UseCinemaBorders;
	bool UseWatermark;

	// Cam params
	float Fov;
};

#define LINEAR_INTERPOLATION_TYPE 1
#define SPLINE_INTERPOLATION_TYPE 2
#define DISABLE_INTERPOLATION 0
/////////////////////////////////////////////////////////////////
extern FrameData FramesArray[MAX_FRAMES_COUNT];
extern string_path demo_file_name;
extern u32 TotalFramesCount;
/////////////////////////////////////////////////////////////////
extern float g_fGlobalFov;
extern float g_fFov;
extern float3 g_vGlobalDepthOfFieldParameters;
extern float3 g_fDOF;
extern float g_fDiaphragm;
extern float g_fGlobalDiaphragm;
extern bool g_bAutofocusEnabled;
extern bool g_bGridEnabled;
extern bool g_bBordersEnabled;
extern bool g_bWatermarkEnabled;
/////////////////////////////////////////////////////////////////
extern void SetDefaultParameters();
extern void ApplyFrameParameters(u32 frame, float interpolation_factor);
extern void ResetParameters();
/////////////////////////////////////////////////////////////////
extern void SaveAllFramesDataToIni(int FramesCount);
extern void ReadAllFramesDataFromIni(const char* name);
/////////////////////////////////////////////////////////////////
extern float4x4 MakeCameraMatrixFromFrameNumber(int Frame);
extern bool NeedInterpolation(int Frame);
extern u32 GetInterpolationType(u32 frame);
/////////////////////////////////////////////////////////////////
