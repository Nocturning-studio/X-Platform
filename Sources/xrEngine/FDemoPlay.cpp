// CDemoPlay.cpp: implementation of the CDemoPlay class.
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "fdemoplay.h"
#include "xr_ioconsole.h"
#include "motion.h"
#include "Render.h"
#include "CameraManager.h"
#include "demo_common.h"
#include "gamefont.h"
#include "Engine.h"
#include "xr_input.h"
#include "igame_level.h"
#include "iinputreceiver.h"
#include "../xrGame/Level.h"
#include "CustomHUD.h"
#include <ctime> // Для даты в отчете
//////////////////////////////////////////////////////////////////////
CDemoPlay::CDemoPlay(const char* name, float ms, u32 cycles, float life_time) : CEffectorCam(cefDemo, life_time)
{
	// Есть ли файл
	if (!FS.exist(name))
	{
		Msg("Can't find file: %s", name);
		g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
		return;
	}

	strcpy(demo_file_name, name);
	ReadAllFramesDataFromIni(demo_file_name);
	Msg("*** Playing demo: %s", demo_file_name);

	m_frames_count = TotalFramesCount;
	Log("~ Total key-frames: ", m_frames_count);

	// Защита на случай если файл пришел пустым
	if (m_frames_count == 0)
	{
		Msg("File corrupted: frames count is zero");
		g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
		return;
	}

	// Определение режима бенчмарка через параметры запуска
	m_bBenchmarkMode = true; //(strstr(Core.Params, "-benchmark") != nullptr);

	// Захват инпута с клавиатуры и мыши
	IR_Capture();

	// Отключение лишнего
	m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
	psHUD_Flags.set(HUD_DRAW, false);

	m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
	psHUD_Flags.set(HUD_CROSSHAIR, false);

	// Скорость и количество повторов демо
	fSpeed = ms;
	dwCyclesLeft = cycles ? cycles : 1;

	if (strstr(Core.Params, "-loop_demo"))
		dwCyclesLeft = 999;

	// Запущен ли сбор общей статистики (для бенчмарка)
	stat_started = FALSE;

	m_OriginalViewMatrix.set(Device.mView);

	// Сет дефолтных значений параметров, которые будут читаться из секции кадра
	SetDefaultParameters();

	// Переменные для окна конца бенчмарка
	bNeedDrawResults = false;
	bNeedToTakeStatsResoultScreenShot = false;
	uTimeToQuit = 0;
	uTimeToScreenShot = 0;

	// Переменные для сбора общей и покадровой статистики
	ResetPerFrameStatistic();

	// Флаг, для того чтобы на первом кадре
	// поставить камеру на нужное место
	// чтобы не сломались транформы
	m_bIsFirstFrame = true;
}

CDemoPlay::~CDemoPlay()
{
	// Отпускаем инпут
	IR_Release();

	// Возвращаем индикаторы обратно
	psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
	psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);

	// Сет дефолтных значений параметров, которые установлены из секции кадра
	ResetParameters();

	// Восстанавливаем оригинальную матрицу камеры
	Device.mView.set(m_OriginalViewMatrix);
	Device.mFullTransform.mul(Device.mProject, Device.mView);

	// Обновляем камеру в менеджере камер
	// g_pGameLevel->Cameras().Update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
}

// -----------------------------------------------------------------------------------------
// Вспомогательная функция для получения матрицы (предполагаем, что она у вас есть)
// Если нет, используйте вашу MakeCameraMatrixFromFrameNumber
Fmatrix CDemoPlay::GetFrameMatrix(int frame)
{
	// Защита от выхода за границы
	if (frame < 0)
		frame = 0;
	if (frame >= m_frames_count)
		frame = m_frames_count - 1;
	return MakeCameraMatrixFromFrameNumber(frame);
}

// Проверка на разрыв (телепорт)
bool CDemoPlay::IsCut(int frame)
{
	if (frame < 0 || frame >= m_frames_count)
		return true;
	// Если тип интерполяции DISABLE, значит этот кадр - точка разрыва
	return GetInterpolationType(frame) == DISABLE_INTERPOLATION;
}


void spline1(float t, Fvector* p, Fvector* ret)
{
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];
	ret->x = 0.0f;
	ret->y = 0.0f;
	ret->z = 0.0f;

	m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
	m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
	m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
	m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

	for (int i = 0; i < 4; i++)
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

void CDemoPlay::MoveCameraSpline(float t, int i0, int i1, int i2, int i3)
{
	Fmatrix m0 = GetFrameMatrix(i0);
	Fmatrix m1 = GetFrameMatrix(i1);
	Fmatrix m2 = GetFrameMatrix(i2);
	Fmatrix m3 = GetFrameMatrix(i3);

	for (int i = 0; i < 4; i++)
	{
		Fvector v[4];
		// Собираем векторы из строк матриц (Row-major в X-Ray?)
		// Если камера ведет себя странно, попробуйте mX.c, mX.k и т.д.
		// Но оставляю ваш метод доступа через массив для совместимости
		v[0].set(m0.m[i][0], m0.m[i][1], m0.m[i][2]);
		v[1].set(m1.m[i][0], m1.m[i][1], m1.m[i][2]);
		v[2].set(m2.m[i][0], m2.m[i][1], m2.m[i][2]);
		v[3].set(m3.m[i][0], m3.m[i][1], m3.m[i][2]);

		spline1(t, v, (Fvector*)&m_Camera.m[i][0]);
	}
}

void linearInterpolate(float t, Fvector* p0, Fvector* p1, Fvector* ret)
{
	ret->x = (1 - t) * p0->x + t * p1->x;
	ret->y = (1 - t) * p0->y + t * p1->y;
	ret->z = (1 - t) * p0->z + t * p1->z;
}

void CDemoPlay::MoveCameraLinear(float t, int i1, int i2)
{
	Fmatrix m1 = GetFrameMatrix(i1);
	Fmatrix m2 = GetFrameMatrix(i2);

	for (int i = 0; i < 4; i++)
	{
		Fvector p0, p1;
		p0.set(m1.m[i][0], m1.m[i][1], m1.m[i][2]);
		p1.set(m2.m[i][0], m2.m[i][1], m2.m[i][2]);

		linearInterpolate(t, &p0, &p1, (Fvector*)&m_Camera.m[i][0]);
	}
}

void CDemoPlay::MoveCamera(u32 frame, float k, int interpolation_type)
{
	// Базовые индексы
	int i0 = frame - 1;
	int i1 = frame;
	int i2 = frame + 1;
	int i3 = frame + 2;

	// Коррекция границ
	if (i0 < 0)
		i0 = i1;
	if (i2 >= m_frames_count)
		i2 = i1;
	if (i3 >= m_frames_count)
		i3 = i2;

	// --- ГЛАВНОЕ ИСПРАВЛЕНИЕ ---
	// Проверяем, является ли точка назначения (i2) точкой разрыва.
	// Если кадр i2 имеет тип 0 (DISABLE), значит мы не должны к нему плавно ехать.
	// Переход должен случиться мгновенно при смене кадра, а пока мы в кадре i1 - стоим на месте.
	bool targetIsCut = IsCut(i2);

	if (targetIsCut)
	{
		// Подменяем целевые точки на текущую.
		// Вместо интерполяции A -> B, будет интерполяция A -> A (стоять на месте).
		i2 = i1;
		i3 = i1;
		// i0 не трогаем, чтобы сохранить касательную прибытия в i1 (для плавности остановки)
	}
	else
	{
		// Стандартная защита сплайна от перегибов, если i3 - это уже следующий разрыв
		if (IsCut(i3))
			i3 = i2;
		if (IsCut(i0))
			i0 = i1; // На всякий случай
	}

	switch (interpolation_type)
	{
	case SPLINE_INTERPOLATION_TYPE:
		MoveCameraSpline(k, i0, i1, i2, i3);
		break;

	case LINEAR_INTERPOLATION_TYPE:
		MoveCameraLinear(k, i1, i2);
		break;

	case DISABLE_INTERPOLATION:
	default:
		m_Camera.set(GetFrameMatrix(i1));
		break;
	}

	// Нормализация матрицы (обязательно для устранения искажений)
	m_Camera.k.normalize();
	Fvector Y = m_Camera.j;
	m_Camera.i.crossproduct(Y, m_Camera.k);
	m_Camera.i.normalize();
	m_Camera.j.crossproduct(m_Camera.k, m_Camera.i);
	m_Camera.j.normalize();
}
// -----------------------------------------------------------------------------------------

// Апдейт камеры
void CDemoPlay::Update(SCamEffectorInfo& info)
{
	// 1. Бенчмарк
	if (m_bBenchmarkMode)
	{
		if (bNeedDrawResults)
			PrintSummaryBenchmarkStatistic();
		else
			ShowPerFrameStatistic();
	}

	// 2. Время
	fStartTime += Device.fTimeDelta;

	// --- ПРОПУСК ВРЕМЕНИ ДЛЯ ТЕЛЕПОРТОВ ---
	// Вычисляем, в каком мы сейчас кадре
	int currentFrame = iFloor(fStartTime / fSpeed);

	// Если текущий кадр существует и он помечен как DISABLE (тип 0)
	if (currentFrame < m_frames_count && !NeedInterpolation(currentFrame))
	{
		// Это значит мы достигли точки разрыва (Frame 10).
		// Не нужно ждать, пока пройдет время этого кадра (fSpeed).
		// Сразу перематываем время на начало следующего кадра (+ чуть-чуть для надежности)
		fStartTime = (float)(currentFrame + 1) * fSpeed + 0.0001f;
	}

	// --- Расчет факторов ---
	float ip;
	float p = fStartTime / fSpeed;
	float InterpolationFactor = modff(p, &ip);
	int Frame = iFloor(ip);

	// --- Логика конца демо ---
	if (m_bBenchmarkMode && Frame == (m_frames_count - 10) && !bNeedDrawResults)
		EnableBenchmarkResultPrint();

	if (Frame >= m_frames_count)
	{
		if (m_bBenchmarkMode)
		{
			if (strstr(Core.Params, "-loop_demo"))
			{
				ResetPerFrameStatistic();
				Frame = 0;
				fStartTime = 0.001f;
			}
			else
			{
				Console->Execute("quit");
				return;
			}
		}
		else
		{
			dwCyclesLeft--;
			if (0 == dwCyclesLeft)
			{
				Close();
				return;
			}
			else
			{
				Frame = 0;
				fStartTime = 0;
			}
		}
	}

	if (Frame >= m_frames_count)
		return;

	ApplyFrameParameters(Frame, InterpolationFactor);

	// --- ЛОГИКА ВЫЗОВА ---
	// Вызываем движение. Все проверки "ехать или стоять" теперь внутри MoveCamera
	if (NeedInterpolation(Frame) && (Frame + 1 < m_frames_count) && !m_bIsFirstFrame)
	{
		// Можно добавить Ease-Out, если следующий кадр - разрыв
		if (!NeedInterpolation(Frame + 1))
		{
			// Cubic Ease-Out
			float t = InterpolationFactor;
			t = 1.0f - powf(1.0f - t, 3.0f);
			MoveCamera(Frame, t, GetInterpolationType(Frame));
		}
		else
		{
			MoveCamera(Frame, InterpolationFactor, GetInterpolationType(Frame));
		}
	}
	else
	{
		MoveCamera(Frame, 0.0f, DISABLE_INTERPOLATION);
	}

	info.n.set(m_Camera.j);
	info.d.set(m_Camera.k);
	info.p.set(m_Camera.c);
	info.fFov = g_fFov;

	fLifeTime -= Device.fTimeDelta;

	// Скриншоты...
	if (m_bBenchmarkMode && bNeedDrawResults)
	{
		if ((Device.dwTimeGlobal >= uTimeToScreenShot) && bNeedToTakeStatsResoultScreenShot)
		{
			bNeedToTakeStatsResoultScreenShot = false;
			Screenshot();
			SaveBenchmarkResults();
		}
		if (Device.dwTimeGlobal >= uTimeToQuit && !strstr(Core.Params, "-loop_demo"))
			Console->Execute("quit");
	}
}

BOOL CDemoPlay::ProcessCam(SCamEffectorInfo& info)
{
	// skeep a few frames before counting
	if (Device.dwPrecacheFrame)
		return TRUE;

	// Защита на случай если файл придет пустой
	if (m_frames_count == NULL)
		Close();

	Update(info);

	m_bIsFirstFrame = false;

	return TRUE;
}

void CDemoPlay::Screenshot()
{
	Render->Screenshot();
}

void CDemoPlay::EnableBenchmarkResultPrint()
{
	// Не запускать повторно
	if (bNeedDrawResults)
		return;

	uTimeToQuit = Device.dwTimeGlobal + 5000;
	uTimeToScreenShot = Device.dwTimeGlobal + 1000;
	bNeedDrawResults = true;
	bNeedToTakeStatsResoultScreenShot = true;
	fLifeTime = 1000; // Продлеваем жизнь эффектора, чтобы успеть показать статы
}

void CDemoPlay::SaveBenchmarkResults()
{
	// Формируем имя файла
	string_path fileName;
	string_path timeStr;

	// Получаем текущее время
	time_t t = time(0);
	struct tm* now = localtime(&t);
	sprintf(timeStr, "%02d-%02d-%02d_%02d-%02d", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour,
			now->tm_min);

	// Имя файла: benchmark_demoName_DateTime.txt
	// Сохраняем в папку логов ($logs$)
	strconcat(sizeof(fileName), fileName, "benchmark_", demo_file_name, "_", timeStr, ".txt");

	IWriter* W = FS.w_open("$logs$", fileName);
	if (!W)
	{
		Msg("! Error: Cannot create benchmark result file: %s", fileName);
		return;
	}

	string1024 tmp;

	// Заголовок
	W->w_string("======================================================================");
	sprintf(tmp, " BENCHMARK REPORT: %s", demo_file_name);
	W->w_string(tmp);
	sprintf(tmp, " Date: %02d.%02d.%04d Time: %02d:%02d:%02d", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900,
			now->tm_hour, now->tm_min, now->tm_sec);
	W->w_string(tmp);
	W->w_string("======================================================================");
	W->w_string("");

	// Инфо о системе
	W->w_string("[ System Information ]");
	// GPU
	sprintf(tmp, " GPU: %s", HW.Caps.id_description);
	W->w_string(tmp);
	// CPU - В движке X-Ray обычно доступно через CPU::ID.cpu_name или аналог,
	// но если его нет под рукой, оставим GPU.

	W->w_string("");

	// Настройки рендера
	W->w_string("[ Display Settings ]");
	sprintf(tmp, " Resolution: %dx%d", Device.dwWidth, Device.dwHeight);
	W->w_string(tmp);
	// Можно добавить версию рендера, если доступна (статическое/динамическое освещение)
	W->w_string("");

	// Результаты
	W->w_string("[ Results ]");

	u32 dwFramesTotal = Device.dwFrame - stat_StartFrame;
	float stat_total_time = stat_Timer_total.GetElapsed_sec();

	sprintf(tmp, " Total Frames:    %u", dwFramesTotal);
	W->w_string(tmp);
	sprintf(tmp, " Total Time:      %.3f s", stat_total_time);
	W->w_string(tmp);

	W->w_string("");

	sprintf(tmp, " FPS Average:     %.2f", fFPS_avg);
	W->w_string(tmp);
	sprintf(tmp, " FPS Max:         %.2f", fFPS_max);
	W->w_string(tmp);
	sprintf(tmp, " FPS Min:         %.2f", fFPS_min);
	W->w_string(tmp);

	W->w_string("");
	W->w_string("======================================================================");

	// Закрываем файл
	FS.w_close(W);

	Msg("Benchmark results saved to: %s", fileName);
}

void CDemoPlay::Close()
{
	g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
	fLifeTime = -1;
}

// Обработчик нажатий клавиш клавиатуры
void CDemoPlay::IR_OnKeyboardPress(int dik)
{
	// Рантайм проверка вместо #ifdef BENCHMARK_BUILD
	if (m_bBenchmarkMode)
	{
		if (dik == DIK_ESCAPE)
			EnableBenchmarkResultPrint(); // В бенчмарке ESC завершает тест с показом результатов
	}
	else
	{
		if (dik == DIK_ESCAPE)
			Close(); // В обычном режиме просто закрывает демо

		if (dik == DIK_GRAVE)
			Console->Show();
	}

	if (dik == DIK_F12)
		Screenshot();
}

void CDemoPlay::PrintSummaryBenchmarkStatistic()
{
	// Выравниваем надпись по центру
	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alCenter);
	Engine.FontManager.GetSystemFont()->OutSetI(0.0, -0.2f);

	Engine.FontManager.GetSystemFont()->OutNext("Benchmark results");

	ChooseTextColor(fFPS_max);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Maximal: %f", fFPS_max);

	ChooseTextColor(fFPS_avg);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Average: %f", fFPS_avg);

	ChooseTextColor(fFPS_min);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Minimal: %f", fFPS_min);

	Engine.FontManager.GetSystemFont()->SetColor(color_rgba(255, 255, 255, 255));
	Engine.FontManager.GetSystemFont()->OutNext("GPU: %s", HW.Caps.id_description);

	if (Device.dwTimeGlobal > uTimeToScreenShot)
		Engine.FontManager.GetSystemFont()->OutNext("Results saved to screenshots and log folder");
}

void CDemoPlay::ResetPerFrameStatistic()
{
	fFPS = 0.0f;
	fFPS_min = flt_max;
	fFPS_max = flt_min;
	fFPS_avg = 0.0f;

	stat_table.clear(); // Очищаем вектор статистики
	stat_StartFrame = Device.dwFrame;
	stat_Timer_total.Start();
}

// Разные цвета для разных значений кадров в секунду
void CDemoPlay::ChooseTextColor(float FPSValue)
{
	if (FPSValue > 50.0f)
		Engine.FontManager.GetSystemFont()->SetColor(color_rgba(101, 255, 0, 200));
	else if (FPSValue < 50.0f && FPSValue > 24.0f)
		Engine.FontManager.GetSystemFont()->SetColor(color_rgba(230, 255, 130, 200));
	else
		Engine.FontManager.GetSystemFont()->SetColor(color_rgba(255, 59, 0, 200));
}

// Статистика в левом верхнем углу экрана
void CDemoPlay::ShowPerFrameStatistic()
{
	// Считаем время кадра
	float fps = 1.f / Device.fTimeDelta;
	float fOne = 0.3f;
	float fInv = 1.f - fOne;
	fFPS = fInv * fFPS + fOne * fps;

	// Добавляем текущий FPS в таблицу для истории (на случай вычисления 1% low)
	stat_table.push_back(fps);

	// Средний FPS
	float stat_total = stat_Timer_total.GetElapsed_sec();
	u32 dwFramesTotal = Device.dwFrame - stat_StartFrame;

	if (stat_total > 0.001f)
		fFPS_avg = float(dwFramesTotal) / stat_total;

	// Фильтр аномально высоких значений (например, при Alt-Tab)
	if (fFPS_max > 256.0f)
		fFPS_max = 60.0f;

	// Обновление мин/макс
	// Пропускаем первые несколько секунд стабилизации, если нужно,
	// но здесь просто фильтруем явные нули
	if (fps > 1.0f)
	{
		if (fFPS < fFPS_min)
			fFPS_min = fFPS;
		if (fFPS > fFPS_max)
			fFPS_max = fFPS;
	}

	// FPS средний не может быть больше FPS максимального (защита от сбоя таймера)
	if (fFPS_avg > fFPS_max && fFPS_max > 0)
	{
		fFPS_avg = fFPS_max;
		stat_StartFrame = Device.dwFrame;
		stat_Timer_total.Start();
	}

	// Выравниваем надпись по левому краю строки
	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);

	if (g_bBordersEnabled)
		Engine.FontManager.GetSystemFont()->OutSetI(-1.0, -0.8f);
	else
		Engine.FontManager.GetSystemFont()->OutSetI(-1.0, -1.0f);

	ChooseTextColor(fFPS);
	Engine.FontManager.GetSystemFont()->OutNext("FPS: %.2f", fFPS);

	ChooseTextColor(fFPS_max);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Maximal: %.2f", fFPS_max);

	ChooseTextColor(fFPS_avg);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Average: %.2f", fFPS_avg);

	ChooseTextColor(fFPS_min);
	Engine.FontManager.GetSystemFont()->OutNext("FPS Minimal: %.2f", fFPS_min);

	Engine.FontManager.GetSystemFont()->SetColor(color_rgba(200, 200, 200, 255));
	Engine.FontManager.GetSystemFont()->OutNext("GPU: %s", HW.Caps.id_description);
}
//////////////////////////////////////////////////////////////////////
