#include "stdafx.h"
#include "Rain.h"
#include "igame_persistent.h"
#include "environment.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "render.h"
#include "igame_level.h"
#include "xr_area.h"
#include "xr_object.h"
#endif
//////////////////////////////////////////////////////////////////////
// Xorshift RNG - очень быстрый и не требует глобального состояния
//////////////////////////////////////////////////////////////////////
struct FastRandom
{
	u32 state;
	FastRandom(u32 seed) : state(seed)
	{
		if (state == 0)
			state = 123456789;
	}

	IC float randF(float min, float max)
	{
		return min + randF() * (max - min);
	}

	IC float randF()
	{ // 0..1
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return float(state) * (1.0f / 4294967296.0f);
	}

	IC int randI(int max)
	{ // 0..max-1
		return iFloor(randF() * max);
	}
};
//////////////////////////////////////////////////////////////////////
// UV Coordinates for drop animation
static Fvector2 s_drops_uv[2][4] = {{{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffect_Rain::CEffect_Rain()
{
	m_state = stIdle;

	m_snd_ambient.create("ambient\\rain", st_Effect, sg_Undefined);

	IReader* F = FS.r_open("$game_meshes$", "dm\\rain.dm");
	VERIFY3(F, "Can't open file.", "dm\\rain.dm");

	m_dm_drop = ::Render->model_CreateDM(F);

	// Load shaders and geoms
	m_sh_rain.create("effects\\rain", "fx\\fx_rain");
	m_geom_rain.create(FVF::F_LIT, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);
	m_geom_drops.create(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, RenderBackend.Vertex.Buffer(),
						RenderBackend.Index.Buffer());

	InitParticlePool();
	FS.r_close(F);

	// Инициализируем индекс
	m_front_buffer_idx = 0;

	// Резервируем память сразу, чтобы избежать аллокаций на старте
	m_render_buffers[0].reserve(MAX_DESIRED_DROPS);
	m_render_buffers[1].reserve(MAX_DESIRED_DROPS);
}

CEffect_Rain::~CEffect_Rain()
{
	m_snd_ambient.destroy();

	DestroyParticlePool();
	::Render->model_Delete(m_dm_drop);
}

// ===========================================================================================
// SPAWN LOGIC
// ===========================================================================================

void CEffect_Rain::SpawnDrop(RainDrop& dest, float radius, FastRandom& R)
{
	// -------------------------------------------------------------------------
	// 1. Параметры окружения (Чтение Environment безопасно)
	// -------------------------------------------------------------------------
	CEnvDescriptorMixer* env = g_pGamePersistent->Environment().CurrentEnv;
	float wind_strength = env->wind_strength;
	float wind_direction = env->wind_direction;

	// -------------------------------------------------------------------------
	// 2. Расчет угла падения (Tilt)
	// -------------------------------------------------------------------------
	// Чем сильнее ветер, тем острее угол
	float tilt_factor = 1.0f - (wind_strength * 0.6f);
	float angle_deg = 90.0f * tilt_factor;
	clamp(angle_deg, 55.0f, 90.0f);

	// Матрица вращения для направления падения
	Fmatrix m_rotate;
	float rot_x = deg2rad(angle_deg);
	float rot_y = -(wind_direction + PI_DIV_2);
	m_rotate.setXYZi(rot_x, rot_y, 0.0f);

	// Устанавливаем направление
	dest.D.set(m_rotate.k);

	// -------------------------------------------------------------------------
	// 3. Расчет позиции спавна (Volume Spawning)
	// -------------------------------------------------------------------------

	// Случайная высота от 5м до потолка (SOURCE_OFFSET)
	// Используем R.randF() вместо ::Random
	float min_h = 5.0f;
	float max_h = SOURCE_OFFSET;
	float spawn_h = min_h + R.randF() * (max_h - min_h);

	// Смещение "против ветра" (Upwind Offset).
	// Чтобы капли, рожденные высоко, падали в поле зрения камеры,
	// их нужно сдвинуть навстречу ветру.
	float wind_shift_dist = spawn_h / tanf(deg2rad(angle_deg));

	Fvector wind_shift_dir;
	wind_shift_dir.setHP(wind_direction, 0.0f);
	wind_shift_dir.mul(-wind_shift_dist);

	// Равномерное распределение по кругу
	// sqrt нужен для равномерной плотности площади круга
	float dist = radius * _sqrt(R.randF());
	float ang = R.randF(0.0f, PI_MUL_2);

	Fvector offset;
	offset.set(dist * _cos(ang), 0.f, dist * _sin(ang));

	// Финальная позиция = Камера + Круг + Смещение ветра + Высота
	Fvector& cam_pos = Engine.RenderView.Position;
	dest.P.set(cam_pos.x + offset.x + wind_shift_dir.x, cam_pos.y + spawn_h, cam_pos.z + offset.z + wind_shift_dir.z);

	// -------------------------------------------------------------------------
	// 4. Скорость
	// -------------------------------------------------------------------------
	dest.fSpeed = R.randF(DROP_SPEED_MIN, DROP_SPEED_MAX) * (1.0f + wind_strength * 0.25f);

	// -------------------------------------------------------------------------
	// 5. Трассировка луча (RayTrace)
	// -------------------------------------------------------------------------
	float check_dist = MAX_DROP_DISTANCE * 1.5f;
	float hit_dist = check_dist;

	// ВАЖНО: rqtStatic - самый безопасный вариант для многопоточности.
	// Мы проверяем только геометрию уровня. Дождь сквозь динамику (акторов) - это
	// допустимый компромисс ради скорости и стабильности.
	BOOL b_hit = RayTrace(dest.P, dest.D, hit_dist, collide::rqtStatic);

	// -------------------------------------------------------------------------
	// 6. Расчет времени жизни
	// -------------------------------------------------------------------------

	// Выбираем случайный UV-сет (текстуру капли)
	dest.uv_set = R.randI(2);

	// Время полета в миллисекундах до точки удара (или конца дистанции)
	float time_to_fly = 1000.0f * hit_dist / dest.fSpeed;

	// Получаем время потокобезопасно (Engine.TimeManager читает атомарные переменные или константы кадра)
	u32 cur_time = Engine.TimeManager.GetGlobalTimeMs();
	u32 delta = Engine.TimeManager.GetDeltaTimeMs();

	if (b_hit)
	{
		// Капля ударится о землю
		dest.dwTime_Life = cur_time + iFloor(time_to_fly) - delta;
		dest.dwTime_Hit = dest.dwTime_Life; // Время удара совпадает со смертью

		// Точка удара для спавна брызг
		dest.Phit.mad(dest.P, dest.D, hit_dist);
	}
	else
	{
		// Капля улетит в никуда (не попала в геометрию)
		dest.dwTime_Life = cur_time + iFloor(time_to_fly) - delta;

		// Время удара ставим далеко в будущее, чтобы splash никогда не сработал
		dest.dwTime_Hit = cur_time + iFloor(2.0f * time_to_fly) - delta;
		dest.Phit.set(dest.P); // Значение не важно
	}
}

BOOL CEffect_Rain::RayTrace(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt)
{
#ifdef _EDITOR
	Tools->RayPick(s, d, range);
	return TRUE;
#else
	if (!g_pGameLevel)
		return FALSE;

	collide::rq_result RQ;
	CObject* E = g_pGameLevel->CurrentViewEntity();
	BOOL res = g_pGameLevel->ObjectSpace.RayPick(s, d, range, tgt, RQ, E);
	if (res)
		range = RQ.range;
	return res;
#endif
}

void CEffect_Rain::SimulateDrops(float dt)
{
	PROFILE_FUNCTION();

	CEnvDescriptorMixer* env = g_pGamePersistent->Environment().CurrentEnv;
	float factor = env->rain_density;
	if (factor < EPS_L)
		return;

	u32 desired_items = iFloor(0.5f * (1.f + factor) * float(MAX_DESIRED_DROPS));

	// Инициализация при старте (оставляем как есть, это один раз)
	{
		FastRandom R(123); // Временный рандом для инита
		while (m_drops.size() < desired_items)
		{
			RainDrop one;
			SpawnDrop(one, SOURCE_RADIUS, R);
			m_drops.push_back(one);
		}
	}

	auto& write_queue = GetWriteBuffer();
	write_queue.clear();
	if (write_queue.capacity() < m_drops.size())
		write_queue.reserve(m_drops.size());

	const Fvector& view_pos = Engine.RenderView.Position;
	u32 global_time = Engine.TimeManager.GetGlobalTimeMs();
	float radius_wrap_sqr = _sqr((SOURCE_RADIUS + 2.0f));

	// 1. Локальные буферы для Render Items
	concurrency::combinable<xr_vector<RainDrawParam>> local_render_buffers;

	// 2. Локальные буферы для отложенных Splash (координаты)
	// Чтобы не лочить particle system
	concurrency::combinable<xr_vector<Fvector>> local_splash_queue;

	// ПАРАЛЛЕЛЬНЫЙ ЦИКЛ БЕЗ LOCK_GUARD
	concurrency::parallel_for(size_t(0), m_drops.size(), [&](size_t i) {
		// Создаем seed на основе индекса и времени, чтобы у каждого потока/капли был уникальный рандом
		FastRandom R(u32(i + global_time));

		RainDrop& drop = m_drops[i];
		bool respawn_needed = false;

		// Check Life
		if (drop.dwTime_Life < global_time)
			respawn_needed = true;

		// Physics
		drop.P.mad(drop.D, drop.fSpeed * dt);

		// Check Bounds
		if (!respawn_needed)
		{
			if (drop.P.distance_to_sqr(view_pos) > radius_wrap_sqr)
				respawn_needed = true;
			else if ((drop.P.y - view_pos.y) < SINK_OFFSET)
				respawn_needed = true;
		}

		// RESPAWN (NO MUTEX NEEDED NOW!)
		if (respawn_needed)
		{
			// Теперь это безопасно, так как R - локальный, а drop[i] уникален для потока
			SpawnDrop(drop, SOURCE_RADIUS, R);
		}

		// SPLASH LOGIC (DEFERRED)
		if (drop.dwTime_Hit < global_time)
		{
			if (drop.Phit.distance_to_sqr(view_pos) < 400.0f)
			{
				// Вместо SpawnSplash добавляем позицию в локальную очередь
				local_splash_queue.local().push_back(drop.Phit);
			}
		}

		// RENDER CULLING
		float speed_factor = drop.fSpeed / DROP_SPEED_MIN;
		float len = 3.5f * speed_factor;
		Fvector pos_head = drop.P;
		Fvector pos_trail;
		pos_trail.mad(pos_head, drop.D, -len);

		Fvector center;
		center.sub(pos_head, pos_trail);
		center.mul(0.5f);
		float radius = center.magnitude();
		center.add(pos_trail);

		if (::Render->ViewBase.testSphere_dirty(center, radius))
		{
			auto& local_vec = local_render_buffers.local();
			// Микро-оптимизация: проверка empty дешевле reserve каждый раз
			if (local_vec.empty())
				local_vec.reserve(128);

			local_vec.emplace_back();
			RainDrawParam& item = local_vec.back();

			item.PosHead = pos_head;
			item.PosTrail = pos_trail;
			u32 s = drop.uv_set;
			// Упрощенный доступ, если компилятор тупит
			item.UV[0] = s_drops_uv[s][0];
			item.UV[1] = s_drops_uv[s][1];
			item.UV[2] = s_drops_uv[s][2];
			item.UV[3] = s_drops_uv[s][3];
		}
	});

	// 4. Слияние результатов (Merge) - это очень быстро
	{
		OPTICK_EVENT("Merge Queues");

		// Сливаем рендер
		local_render_buffers.combine_each([&](const xr_vector<RainDrawParam>& local_vec) {
			if (!local_vec.empty())
			{
				write_queue.insert(write_queue.end(), local_vec.begin(), local_vec.end());
			}
		});

		// Сливаем и обрабатываем сплеши (уже в одном потоке!)
		local_splash_queue.combine_each([&](const xr_vector<Fvector>& local_splashes) {
			for (const auto& pos : local_splashes)
			{
				SpawnSplash(pos); // Теперь безопасно вызываем оригинальный метод с партиклами
			}
		});
	}
}

// ===========================================================================================
// MAIN UPDATE / RENDER LOOPS
// ===========================================================================================

void __stdcall CEffect_Rain::MT_CALC()
{
	// Вызываем нашу логику, используя сохраненный dt
	SimulateDrops(m_worker_dt);
}

void CEffect_Rain::OnFrame()
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (!g_pGameLevel)
		return;
#endif

	float factor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

	// Calculate hemi factor (occlusion by roofs for sound)
	float hemi_factor = 1.f;
#ifndef _EDITOR
	CObject* E = g_pGameLevel->CurrentViewEntity();
	if (E && E->renderable_ROS())
	{
		float lumi = E->renderable_ROS()->get_luminocity_hemi();
		hemi_factor = 1.f - 2.0f * (0.3f - _min(_min(1.f, lumi), 0.3f));
	}
#endif

	// State machine for Sound
	switch (m_state)
	{
	case stIdle:
		if (factor >= EPS_L)
		{
			m_state = stWorking;
			m_snd_ambient.play(0, sm_Looped);
			m_snd_ambient.set_range(SOURCE_OFFSET, SOURCE_OFFSET * 2.f);
		}
		break;
	case stWorking:
		if (factor < EPS_L)
		{
			m_state = stIdle;
			m_snd_ambient.stop();
			return;
		}
		break;
	}

	// Update ambient sound
	if (m_snd_ambient._feedback())
	{
		Fvector snd_pos;
		snd_pos.mad(Engine.RenderView.Position, Fvector().set(0, 1, 0), SOURCE_OFFSET);
		m_snd_ambient.set_position(snd_pos);
		m_snd_ambient.set_volume(1.1f * factor * hemi_factor);
	}

	if (m_state == stWorking)
	{
		// 1. Swap Buffers
		// Меняем буферы местами. Front идет на рендер (с данными прошлого кадра),
		// Back освобождается для записи нового кадра в потоке.
		SwapBuffers();

		// 2. Сохраняем DT
		// В поток нельзя передать аргументы напрямую через этот макрос,
		// поэтому сохраняем dt в член класса.
		m_worker_dt = Engine.TimeManager.GetDeltaTime();

		// 3. Добавляем задачу в ThreadManager
		Engine.ThreadManager.AddParallelTask(CThreadManager::ParallelTask(this, &CEffect_Rain::MT_CALC));
	}
	else
	{
		// Очистка если дождь кончился
		if (!GetReadBuffer().empty())
			GetReadBuffer().clear();
		if (!GetWriteBuffer().empty())
			GetWriteBuffer().clear();
	}
}

void CEffect_Rain::Render()
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (!g_pGameLevel)
		return;
#endif

	float factor = g_pGamePersistent->Environment().CurrentEnv->rain_density;
	if (factor < EPS_L)
		return;

	// Calculate count and color
	u32 desired_items = iFloor(0.5f * (1.f + factor) * float(MAX_DESIRED_DROPS));

	float factor_visual = factor / 2.f + .5f;
	Fvector3 f_rain_color = g_pGamePersistent->Environment().CurrentEnv->rain_color;
	u32 u_rain_color = color_rgba_f(f_rain_color.x, f_rain_color.y, f_rain_color.z, factor_visual);

	// 1. Render Drops
	UpdateAndRenderDrops(desired_items, u_rain_color);

	// 2. Render Splashes
	UpdateAndRenderSplashes(u_rain_color);
}

void CEffect_Rain::UpdateAndRenderDrops(u32 /*desired_items*/, u32 rain_color)
{
	PROFILE_FUNCTION(); // "Draw Phase"

	// Ссылка на буфер ЧТЕНИЯ (Front Buffer)
	const auto& read_queue = GetReadBuffer();

	size_t count = read_queue.size();
	if (count == 0)
		return;

	// Lock Buffer
	u32 v_offset;
	FVF::LIT* verts = (FVF::LIT*)RenderBackend.Vertex.Lock(count * 4, m_geom_rain->vb_stride, v_offset);

	// Вспомогательные
	const Fvector& view_pos = Engine.RenderView.Position;
	Fvector cam_dir, line_dir, line_top, p, center;
	float w = DROP_WIDTH;

	// Просто молотим данные из буфера в видеокарту
	for (const auto& item : read_queue)
	{
		// Билбординг (поворот к камере)
		// Считаем тут, так как позиция камеры могла измениться с момента симуляции (если многопоток)
		center.add(item.PosHead, item.PosTrail);
		center.mul(0.5f);

		cam_dir.sub(center, view_pos);
		cam_dir.normalize();

		line_dir.sub(item.PosHead, item.PosTrail);
		line_dir.normalize();

		line_top.crossproduct(cam_dir, line_dir);

		// Геометрия
		p.mad(item.PosTrail, line_top, -w);
		verts->set(p, rain_color, item.UV[0].x, item.UV[0].y);
		verts++;
		p.mad(item.PosTrail, line_top, w);
		verts->set(p, rain_color, item.UV[1].x, item.UV[1].y);
		verts++;
		p.mad(item.PosHead, line_top, -w);
		verts->set(p, rain_color, item.UV[2].x, item.UV[2].y);
		verts++;
		p.mad(item.PosHead, line_top, w);
		verts->set(p, rain_color, item.UV[3].x, item.UV[3].y);
		verts++;
	}

	RenderBackend.Vertex.Unlock(count * 4, m_geom_rain->vb_stride);

	// Draw Call
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_Shader(m_sh_rain);
	RenderBackend.set_Geometry(m_geom_rain);
	RenderBackend.Render(D3DPT_TRIANGLELIST, v_offset, 0, count * 4, 0, count * 2);
	RenderBackend.set_CullMode(D3DCULL_CCW);
}

void CEffect_Rain::UpdateAndRenderSplashes(u32 rain_color)
{
	PROFILE_FUNCTION();

	SplashParticle* P = m_particle_active;
	if (!P)
		return;

	float dt = Engine.TimeManager.GetDeltaTime();

	RenderBackend.set_Shader(m_dm_drop->shader);

	// Lock Buffers for particles
	u32 v_offset, i_offset;
	u32 max_verts = PARTICLES_CACHE * m_dm_drop->number_vertices;
	u32 max_inds = PARTICLES_CACHE * m_dm_drop->number_indices;

	IRender_DetailModel::fvfVertexOut* v_ptr =
		(IRender_DetailModel::fvfVertexOut*)RenderBackend.Vertex.Lock(max_verts, m_geom_drops->vb_stride, v_offset);
	u16* i_ptr = RenderBackend.Index.Lock(max_inds, i_offset);

	Fmatrix m_transform, m_scale;
	int p_count = 0;

	while (P)
	{
		SplashParticle* next = P->next;

		// Update
		P->time -= dt;
		if (P->time < 0)
		{
			FreeParticle(P);
			P = next;
			continue;
		}

		// Render Culling
		if (::Render->ViewBase.testSphere_dirty(P->bounds.P, P->bounds.R))
		{
			float scale = P->time / PARTICLE_TIME;
			m_scale.scale(scale, scale, scale);
			m_transform.mul_43(P->mTransform, m_scale);

			m_dm_drop->transfer(m_transform, v_ptr, rain_color, i_ptr, p_count * m_dm_drop->number_vertices);
			v_ptr += m_dm_drop->number_vertices;
			i_ptr += m_dm_drop->number_indices;
			p_count++;

			// Batch flush
			if (p_count >= PARTICLES_CACHE)
			{
				u32 prim_count = (p_count * m_dm_drop->number_indices) / 3;
				RenderBackend.Vertex.Unlock(max_verts, m_geom_drops->vb_stride);
				RenderBackend.Index.Unlock(max_inds);

				RenderBackend.set_Geometry(m_geom_drops);
				RenderBackend.Render(D3DPT_TRIANGLELIST, v_offset, 0, p_count * m_dm_drop->number_vertices, i_offset,
									 prim_count);

				// Re-lock
				v_ptr = (IRender_DetailModel::fvfVertexOut*)RenderBackend.Vertex.Lock(
					max_verts, m_geom_drops->vb_stride, v_offset);
				i_ptr = RenderBackend.Index.Lock(max_inds, i_offset);
				p_count = 0;
			}
		}
		P = next;
	}

	// Final flush
	u32 total_verts = p_count * m_dm_drop->number_vertices;
	u32 total_inds = p_count * m_dm_drop->number_indices;

	RenderBackend.Vertex.Unlock(total_verts, m_geom_drops->vb_stride);
	RenderBackend.Index.Unlock(total_inds);

	if (p_count > 0)
	{
		RenderBackend.set_Geometry(m_geom_drops);
		RenderBackend.Render(D3DPT_TRIANGLELIST, v_offset, 0, total_verts, i_offset, total_inds / 3);
	}
}

// ===========================================================================================
// PARTICLE POOL MANAGEMENT
// ===========================================================================================

void CEffect_Rain::SpawnSplash(const Fvector& pos)
{
	if (::Random.randI(2) != 0)
		return;

	SplashParticle* P = AllocateParticle();
	if (!P)
		return;

	P->time = PARTICLE_TIME;
	P->mTransform.rotateY(::Random.randF(PI_MUL_2));
	P->mTransform.translate_over(pos);
	P->mTransform.transform_tiny(P->bounds.P, m_dm_drop->bv_sphere.P);
	P->bounds.R = m_dm_drop->bv_sphere.R;
}

void CEffect_Rain::InitParticlePool()
{
	m_particle_pool.resize(MAX_PARTICLES);
	for (u32 it = 0; it < m_particle_pool.size(); it++)
	{
		SplashParticle& P = m_particle_pool[it];
		P.prev = (it > 0) ? (&m_particle_pool[it - 1]) : nullptr;
		P.next = (it < (m_particle_pool.size() - 1)) ? (&m_particle_pool[it + 1]) : nullptr;
	}

	m_particle_active = nullptr;
	m_particle_idle = &m_particle_pool.front();
}

void CEffect_Rain::DestroyParticlePool()
{
	m_particle_active = nullptr;
	m_particle_idle = nullptr;
	m_particle_pool.clear();
}

void CEffect_Rain::ListRemove(SplashParticle* P, SplashParticle*& LST)
{
	VERIFY(P);
	SplashParticle* prev = P->prev;
	SplashParticle* next = P->next;

	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;

	if (LST == P)
		LST = next;

	P->prev = nullptr;
	P->next = nullptr;
}

void CEffect_Rain::ListInsert(SplashParticle* P, SplashParticle*& LST)
{
	VERIFY(P);
	P->prev = nullptr;
	P->next = LST;
	if (LST)
		LST->prev = P;
	LST = P;
}

CEffect_Rain::SplashParticle* CEffect_Rain::AllocateParticle()
{
	SplashParticle* P = m_particle_idle;
	if (!P)
		return nullptr;

	ListRemove(P, m_particle_idle);
	ListInsert(P, m_particle_active);
	return P;
}

void CEffect_Rain::FreeParticle(SplashParticle* P)
{
	ListRemove(P, m_particle_active);
	ListInsert(P, m_particle_idle);
}
