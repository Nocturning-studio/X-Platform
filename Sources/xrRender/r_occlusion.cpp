#include "StdAfx.h"
#include ".\r_occlusion.h"

R_occlusion::R_occlusion(void)
{
	////OPTICK_EVENT("R_occlusion::R_occlusion");

	enabled = ps_render_flags.test(RFLAG_EXP_HW_OCC);
}

R_occlusion::~R_occlusion(void)
{
	////OPTICK_EVENT("R_occlusion::~R_occlusion");

	occq_destroy();
}

void R_occlusion::occq_create(u32 limit)
{
	////OPTICK_EVENT("R_occlusion::occq_create");

	pool.reserve(limit);
	used.reserve(limit);
	fids.reserve(limit);
	for (u32 it = 0; it < limit; it++)
	{
		_Q q;
		q.order = it;
		if (FAILED(HW.GetDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q.Q)))
			break;
		pool.push_back(q);
	}
	std::reverse(pool.begin(), pool.end());
}

void R_occlusion::occq_destroy()
{
	////OPTICK_EVENT("R_occlusion::occq_destroy");

	while (!used.empty())
	{
		try
		{
			_RELEASE(used.back().Q);
			used.pop_back();
		}
		catch (...)
		{
			Msg("! Failed to release OCCq `used`");
			FlushLog();
		}
	}

	while (!pool.empty())
	{
		try
		{
			_RELEASE(pool.back().Q);
			pool.pop_back();
		}
		catch (...)
		{
			Msg("! Failed to release OCCq `pool`");
			FlushLog();
		}
	}
	used.clear();
	pool.clear();
	fids.clear();
}

u32 R_occlusion::occq_begin(u32& ID)
{
	if (!enabled) return 0;
	RenderImplementation.stats.o_queries++;

	if (pool.empty())
	{
		Msg("! R_occlusion::occq_begin: No available queries in pool");
		ID = 0xffffffff;
		return 0;
	}

	if (!fids.empty())
	{
		ID = fids.back();
		fids.pop_back();
		if (ID >= used.size())
			used.resize(ID + 1);
		_RELEASE(used[ID].Q);
	}
	else
	{
		ID = used.size();
		used.push_back(pool.back());
	}

	used[ID] = pool.back();
	pool.pop_back();

	used[ID].frame_issued = Engine.TimeManager.GetFrameCount(); // сохраняем кадр выдачи

	HRESULT hr = used[ID].Q->Issue(D3DISSUE_BEGIN);
	if (FAILED(hr))
	{
		Msg("! R_occlusion::occq_begin: Failed to issue query [HR:0x%08X]", hr);
		// Возвращаем запрос обратно в пул при ошибке
		pool.push_back(used[ID]);
		used[ID].Q = nullptr;
		fids.push_back(ID);
		ID = 0xffffffff;
		return 0;
	}

	return used[ID].order;
}

void R_occlusion::occq_end(u32& ID)
{
	////OPTICK_EVENT("R_occlusion::occq_end");

	if (!enabled || ID == 0xffffffff || ID >= used.size() || used[ID].Q == nullptr)
		return;

	HRESULT hr = used[ID].Q->Issue(D3DISSUE_END);
	if (FAILED(hr))
	{
		Msg("! R_occlusion::occq_end: Failed to end query [ID:%u, HR:0x%08X]", ID, hr);
	}
}

u32 R_occlusion::occq_get(u32& ID, bool bWait)
{
	if (!enabled)
		return 0xffffffff;

	if (ID >= used.size() || used[ID].Q == nullptr)
	{
		Msg("! R_occlusion::occq_get: Invalid ID or null query pointer [ID:%d, used.size:%d]", ID, used.size());
		return 0xffffffff;
	}

	DWORD fragments = 0;
	HRESULT hr;
	const u32 current_frame = Engine.TimeManager.GetFrameCount();
	const u32 frames_pending = current_frame - used[ID].frame_issued;

	if (bWait)
	{
		// Блокирующий режим (как раньше)
		CTimer T; T.Start();
		Engine.Statistic->RenderDUMP_Wait.Begin();
		while ((hr = used[ID].Q->GetData(&fragments, sizeof(fragments), D3DGETDATA_FLUSH)) == S_FALSE)
		{
			if (!SwitchToThread()) Sleep(ps_r_thread_wait_sleep);
			if (T.GetElapsed_ms() > 500)
			{
				fragments = 0xffffffff;
				break;
			}
		}
		Engine.Statistic->RenderDUMP_Wait.End();
	}
	else
	{
		// Неблокирующая проверка
		hr = used[ID].Q->GetData(&fragments, sizeof(fragments), 0);
		if (hr == S_FALSE)
		{
			// Запрос ещё выполняется. Если висит слишком долго – принудительно сбрасываем.
			if (frames_pending > 3) // более 3 кадров
			{
				Msg("! R_occlusion::occq_get: Query stuck for %d frames, forcing release", frames_pending);
				hr = D3DERR_DEVICELOST; // имитируем потерю устройства, чтобы освободить
				fragments = 0xffffffff;
			}
			else
			{
				return 0xfffffffe; // данные не готовы, запрос остаётся в used
			}
		}
	}

	// Если мы здесь, значит данные получены или произошла ошибка/таймаут.
	// Возвращаем запрос в пул в любом случае.
	if (hr == D3DERR_DEVICELOST)
		fragments = 0xffffffff;

	if (fragments == 0)
		RenderImplementation.stats.o_culled++;

	_Q& Q = used[ID];
	if (pool.empty())
		pool.push_back(Q);
	else
	{
		int it = int(pool.size()) - 1;
		while ((it >= 0) && (pool[it].order < Q.order)) it--;
		pool.insert(pool.begin() + it + 1, Q);
	}

	used[ID].Q = nullptr;
	fids.push_back(ID);
	ID = 0;
	return fragments;
}
