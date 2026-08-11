/////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "demo_common.h"
#include "IGame_Persistent.h"
#include "xr_ioconsole.h"
/////////////////////////////////////////////////////////////////
// Создаем массив из структур с запасом
// Для кажого кадра мы используем свою структуру внутри массива
FrameData FramesArray[MAX_FRAMES_COUNT];

// Имя демо файла
string_path demo_file_name;

u32 TotalFramesCount;
/////////////////////////////////////////////////////////////////
float g_fGlobalFov = 67.5f;
float g_fFov = 67.5f;
fvec3 g_vGlobalDepthOfFieldParameters;
fvec3 g_fDOF;
bool g_bAutofocusEnabled = false;
bool g_bGridEnabled = false;
bool g_bBordersEnabled = false;
bool g_bWatermarkEnabled = false;
/////////////////////////////////////////////////////////////////
void SetDefaultParameters()
{
	g_fGlobalFov = Engine.RenderView.Fov;
	g_fFov = g_fGlobalFov;

	g_pGamePersistent->GetCurrentDof(g_vGlobalDepthOfFieldParameters);
	g_fDOF.set(g_vGlobalDepthOfFieldParameters);
	g_fDOF.z = 1.0f;
	g_pGamePersistent->SetBaseDof(g_fDOF);

	g_bAutofocusEnabled = false;
	g_bGridEnabled = false;
	g_bBordersEnabled = false;
	
	g_bWatermarkEnabled = false;
}

float lerp(float a, float b, float f)
{
	return (a * (1.0f - f)) + (b * f);
}

void ApplyFrameParameters(u32 frame, float interpolation_factor)
{
	u32 ActualFrame = frame;
	u32 NextFrame = ActualFrame + 1;
	
	if (ActualFrame == TotalFramesCount)
		NextFrame -= 1;

	if (!NeedInterpolation(NextFrame))
		NextFrame -= 1;

	float FovActual = FramesArray[ActualFrame].Fov;
	float FovNext = FramesArray[NextFrame].Fov;
	g_fFov = lerp(FovActual, FovNext, interpolation_factor);

	g_fDOF.set(FramesArray[ActualFrame].DOF);
	g_pGamePersistent->SetBaseDof(g_fDOF);

	g_bBordersEnabled = FramesArray[ActualFrame].UseCinemaBorders;

	if (g_bBordersEnabled)
		Console->Execute("r_cinema_borders on");
	else
		Console->Execute("r_cinema_borders off");

	g_bWatermarkEnabled = FramesArray[ActualFrame].UseWatermark;

	if (g_bWatermarkEnabled)
		Console->Execute("r_watermark on");
	else
		Console->Execute("r_watermark off");
}

void ResetParameters()
{
	g_fDOF = g_vGlobalDepthOfFieldParameters;
	g_pGamePersistent->SetBaseDof(g_fDOF);
	g_pGamePersistent->SetPickableEffectorDOF(false);

	g_fFov = g_fGlobalFov;

	g_bAutofocusEnabled = false;
	g_bGridEnabled = false;
	g_bBordersEnabled = false;
	g_bWatermarkEnabled = false;

	Console->Execute("r_photo_grid off");
	Console->Execute("r_cinema_borders off");
	Console->Execute("r_watermark off");

#ifndef MASTER_GOLD
	Console->Execute("r_debug_render disabled");
#endif
}
/////////////////////////////////////////////////////////////////
void SaveAllFramesDataToIni(int FramesCount)
{
	if (FramesCount == 0)
		return;

	TotalFramesCount = FramesCount;

	// Создаем ини файл в который будем сохранять данные
	CInifile ini(demo_file_name, FALSE, FALSE, TRUE);

	// Записываем общее количество ключевых кадров в глобальной секции
	ini.w_u32("global", "frames_count", TotalFramesCount);

	// Итерируемся по массиву, создавая секции с номерами ключевых кадров
	// чтобы записать в них данные
	for (int FramesIterator = 0; FramesIterator < (int)TotalFramesCount; FramesIterator++)
	{
		string_path section_name = "frame_";
		strcat_s(section_name, std::to_string(FramesIterator).c_str());

		ini.w_fvector3(section_name, "hpb", FramesArray[FramesIterator].HPB);
		ini.w_fvector3(section_name, "position", FramesArray[FramesIterator].Position);

		ini.w_u32(section_name, "interpolation_type", FramesArray[FramesIterator].InterpolationType);

		ini.w_fvector3(section_name, "dof", FramesArray[FramesIterator].DOF);

		ini.w_bool(section_name, "cinema_borders", FramesArray[FramesIterator].UseCinemaBorders);
		ini.w_bool(section_name, "watermark", FramesArray[FramesIterator].UseWatermark);

		ini.w_float(section_name, "fov", FramesArray[FramesIterator].Fov);
	}
}

void ReadAllFramesDataFromIni(const char* name)
{
	// Открываем ини файл из которого будем читать данные
	CInifile* ini = xr_new<CInifile>(name, TRUE, TRUE, FALSE);

	// Получаем общее количество ключевых кадров
	TotalFramesCount = ini->r_u32("global", "frames_count");

	// Итерируемся по числу ключевых кадров чтобы найти секции с соответствующими
	// номерами и присвоить данные из строк в них к структуре из массива с таким же номером
	for (int FramesIterator = 0; FramesIterator < (int)TotalFramesCount; FramesIterator++)
	{
		string_path section_name = "frame_";
		strcat_s(section_name, std::to_string(FramesIterator).c_str());

		FramesArray[FramesIterator].HPB = ini->r_fvector3(section_name, "hpb");
		FramesArray[FramesIterator].Position = ini->r_fvector3(section_name, "position");

		FramesArray[FramesIterator].InterpolationType = ini->r_u32(section_name, "interpolation_type");

		FramesArray[FramesIterator].DOF = ini->r_fvector3(section_name, "dof");

		FramesArray[FramesIterator].UseCinemaBorders = ini->r_bool(section_name, "cinema_borders");
		FramesArray[FramesIterator].UseWatermark = ini->r_bool(section_name, "watermark");

		FramesArray[FramesIterator].Fov = ini->r_float(section_name, "fov");
	}
}
/////////////////////////////////////////////////////////////////
fmat4x4 MakeCameraMatrixFromFrameNumber(int Frame)
{
	fvec3 HPB, Position;
	fmat4x4 Camera;

	HPB.set(FramesArray[Frame].HPB);
	Position.set(FramesArray[Frame].Position);

	Camera.setHPB(HPB.x, HPB.y, HPB.z);
	Camera.translate_over(Position);

	return Camera;
}

bool NeedInterpolation(int Frame)
{
	if (FramesArray[Frame].InterpolationType == DISABLE_INTERPOLATION)
		return false;
	else
		return true;
}

u32 GetInterpolationType(u32 frame)
{
	return FramesArray[frame].InterpolationType;
}
/////////////////////////////////////////////////////////////////
