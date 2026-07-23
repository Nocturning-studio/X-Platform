////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph_debug.cpp
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Level graph debug functions
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef DEBUG
#ifndef AI_COMPILER

#include "level_graph.h"
#include "level.h"
#include "game_base_space.h"
#include "hudmanager.h"
#include "xrserver_objects_alife_monsters.h"
#include "alife_simulator.h"
#include "alife_graph_registry.h"
#include "alife_object_registry.h"
#include "alife_human_brain.h"
#include "alife_monster_movement_manager.h"
#include "alife_monster_detail_path_manager.h"

#include "debug_renderer.h"

void CLevelGraph::setup_current_level(const int& level_id)
{
	if (m_current_level_id == level_id)
		return;

	m_current_actual = false;
	m_current_level_id = level_id;
}

void CLevelGraph::render()
{
	if (psAI_Flags.test(aiDrawGameGraph))
	{
		//		if (psHUD_Flags.test(HUD_DRAW))
		draw_game_graph();
	}

	if (!bDebug && !psAI_Flags.test(aiMotion))
		return;

	if (bDebug && psAI_Flags.test(aiDebug))
		draw_nodes();

	draw_restrictions();

	if (psAI_Flags.test(aiCover))
		draw_covers();

	if (!psHUD_Flags.test(HUD_DRAW))
		return;

	if (psAI_Flags.test(aiMotion))
		draw_objects();

#ifdef DEBUG
	draw_debug_node();
#endif
}

void modify(const int& vertex_id, Fbox& bounding_box)
{
	const CGameGraph& graph = ai().game_graph();
	bounding_box.modify(graph.vertex(vertex_id)->game_point());

	CGameGraph::const_iterator I, E;
	graph.begin(vertex_id, I, E);
	for (; I != E; ++I)
		bounding_box.modify(graph.vertex(graph.value(vertex_id, I))->game_point());
}

void CLevelGraph::update_current_info()
{
	m_current_actual = true;

	Fbox bounding_box;
	bounding_box.invalidate();

	bool found = false;
	bool all = (m_current_level_id == -1);
	const CGameGraph& graph = ai().game_graph();
	for (int i = 0, n = (int)graph.header().vertex_count(); i < n; ++i)
	{
		if (!all)
		{
			if (graph.vertex(i)->level_id() != m_current_level_id)
			{
				if (found)
					break;
				continue;
			}
			found = true;
		}

		modify(i, bounding_box);
	}

	bounding_box.getcenter(m_current_center);
	bounding_box.getradius(m_current_radius);
}

fvec3 CLevelGraph::convert_position(const fvec3& position)
{
	fvec3 result = position;
	result.sub(m_current_center, position);
	result.x *= 5.f / m_current_radius.x;
	result.y *= 1.f / m_current_radius.y;
	result.z *= 5.f / m_current_radius.z;

	result.mul(.5f);
	result.add(Level().CurrentEntity()->Position());
	result.y += 4.5f;

	return (result);
}

void CLevelGraph::draw_edge(const int& vertex_id0, const int& vertex_id1)
{
	const float radius = .005f;
	const u32 vertex_color = D3DCOLOR_XRGB(0, 255, 255);
	const u32 edge_color = D3DCOLOR_XRGB(0, 255, 0);

	const CGameGraph& graph = ai().game_graph();
	fvec3 position0 = convert_position(graph.vertex(vertex_id0)->game_point());
	fvec3 position1 = convert_position(graph.vertex(vertex_id1)->game_point());

	CDebugRenderer& render = Level().debug_renderer();
	render.draw_aabb(position0, radius, radius, radius, vertex_color);
	render.draw_aabb(position1, radius, radius, radius, vertex_color);
	render.draw_line(Fidentity, position0, position1, edge_color);
	//	RenderBackend.dbg_DrawAABB		(position0,radius,radius,radius,vertex_color);
	//	RenderBackend.dbg_DrawAABB		(position1,radius,radius,radius,vertex_color);
	//	RenderBackend.dbg_DrawLINE		(Fidentity,position0,position1,edge_color);
}

void CLevelGraph::draw_vertex(const int& vertex_id)
{
	CGameGraph::const_iterator I, E;
	const CGameGraph& graph = ai().game_graph();
	graph.begin(vertex_id, I, E);
	for (; I != E; ++I)
	{
		int neighbour_id = graph.value(vertex_id, I);
		if (neighbour_id < vertex_id)
			draw_edge(vertex_id, neighbour_id);
	}
}

void CLevelGraph::draw_stalkers(const int& vertex_id)
{
	if (!ai().get_alife())
		return;

	const float radius = .0105f;
	const u32 color = D3DCOLOR_XRGB(255, 0, 0);
	const CGameGraph& graph = ai().game_graph();
	CGameFont& font = *HUD().Font().pFontDI;
	fvec3 position = convert_position(graph.vertex(vertex_id)->game_point());

	font.SetColor(D3DCOLOR_XRGB(255, 255, 0));

	bool show_text = true;
	for (;;)
	{
		fvec4 temp;
		Engine.RenderView.ViewProjection.transform(temp, position);
		font.OutSetI(temp.x, -temp.y);
		font.SetHeightI(.05f / _sqrt(temp.w));

		if (temp.z < 0.f)
		{
			show_text = false;
			break;
		}

		if (temp.w < 0.f)
		{
			show_text = false;
			break;
		}

		if (temp.x < -1.f)
		{
			show_text = false;
			break;
		}

		if (temp.x > 1.f)
		{
			show_text = false;
			break;
		}

		if (temp.y < -1.f)
		{
			show_text = false;
			break;
		}

		if (temp.x > 1.f)
		{
			show_text = false;
			break;
		}

		break;
	}

	typedef CALifeGraphRegistry::OBJECT_REGISTRY OBJECT_REGISTRY;
	typedef OBJECT_REGISTRY::_const_iterator const_iterator;
	typedef CALifeMonsterDetailPathManager::PATH PATH;
	const OBJECT_REGISTRY& objects = ai().alife().graph().objects()[vertex_id].objects();

	CDebugRenderer& render = Level().debug_renderer();
	if (show_text)
	{
		bool first_time = true;
		const_iterator I = objects.objects().begin();
		const_iterator E = objects.objects().end();
		for (; I != E; ++I)
		{
			CSE_ALifeDynamicObject* object = (*I).second;
			CSE_ALifeHumanStalker* stalker = smart_cast<CSE_ALifeHumanStalker*>(object);
			if (!stalker)
				continue;

			const PATH& path = stalker->brain().movement().detail().path();
			const float& walked_distance =
				(path.size() < 2) ? 0.f : stalker->brain().movement().detail().walked_distance();
			//			font.OutNext		("%s",stalker->name_replace());

			if ((path.size() >= 2) && !fis_zero(walked_distance))
				continue;

			if (!first_time)
				continue;

			fvec3 position = convert_position(graph.vertex(stalker->m_tGraphID)->game_point());
			render.draw_aabb(position, radius, radius, radius, color);
			first_time = false;
			continue;
		}
	}

	const_iterator I = objects.objects().begin();
	const_iterator E = objects.objects().end();
	for (; I != E; ++I)
	{
		CSE_ALifeDynamicObject* object = (*I).second;
		CSE_ALifeHumanStalker* stalker = smart_cast<CSE_ALifeHumanStalker*>(object);
		if (!stalker)
			continue;

		const PATH& path = stalker->brain().movement().detail().path();
		if (path.size() < 2)
			continue;

		u32 game_vertex_id0 = stalker->m_tGraphID;
		u32 game_vertex_id1 = path[path.size() - 2];
		const float& walked_distance = stalker->brain().movement().detail().walked_distance();

		if (fis_zero(walked_distance))
			continue;

		fvec3 position0 = graph.vertex(game_vertex_id0)->game_point();
		fvec3 position1 = graph.vertex(game_vertex_id1)->game_point();
		const float distance = position0.distance_to(position1);

		position0 = convert_position(position0);
		position1 = convert_position(position1);

		fvec3 direction = fvec3().sub(position1, position0);
		float magnitude = direction.magnitude();
		direction.normalize();
		direction.mul(magnitude * walked_distance / distance);
		direction.add(position0);
		render.draw_aabb(direction, radius, radius, radius, color);

		fvec4 temp;
		Engine.RenderView.ViewProjection.transform(temp, direction);

		if (temp.z < 0.f)
			continue;

		if (temp.w < 0.f)
			continue;

		if (temp.x < -1.f)
			continue;

		if (temp.x > 1.f)
			continue;

		if (temp.y < -1.f)
			continue;

		if (temp.x > 1.f)
			continue;

		font.SetHeightI(.05f / _sqrt(temp.w));
	}
}

void CLevelGraph::draw_objects(const int& vertex_id)
{
	if (!ai().get_alife())
		return;

	const float radius = .0105f;
	const u32 color = D3DCOLOR_XRGB(255, 0, 0);
	const CGameGraph& graph = ai().game_graph();
	CGameFont& font = *HUD().Font().pFontDI;
	fvec3 position = convert_position(graph.vertex(vertex_id)->game_point());

	font.SetColor(D3DCOLOR_XRGB(255, 255, 0));

	bool show_text = true;
	for (;;)
	{
		fvec4 temp;
		Engine.RenderView.ViewProjection.transform(temp, position);
		font.OutSetI(temp.x, -temp.y);
		font.SetHeightI(.05f / _sqrt(temp.w));

		if (temp.z < 0.f)
		{
			show_text = false;
			break;
		}

		if (temp.w < 0.f)
		{
			show_text = false;
			break;
		}

		if (temp.x < -1.f)
		{
			show_text = false;
			break;
		}

		if (temp.x > 1.f)
		{
			show_text = false;
			break;
		}

		if (temp.y < -1.f)
		{
			show_text = false;
			break;
		}

		if (temp.x > 1.f)
		{
			show_text = false;
			break;
		}

		break;
	}

	typedef CALifeGraphRegistry::OBJECT_REGISTRY OBJECT_REGISTRY;
	typedef OBJECT_REGISTRY::_const_iterator const_iterator;
	typedef CALifeMonsterDetailPathManager::PATH PATH;
	const OBJECT_REGISTRY& objects = ai().alife().graph().objects()[vertex_id].objects();

	CDebugRenderer& render = Level().debug_renderer();
	if (show_text)
	{
		bool first_time = true;
		const_iterator I = objects.objects().begin();
		const_iterator E = objects.objects().end();
		for (; I != E; ++I)
		{
			CSE_ALifeDynamicObject* object = (*I).second;
			CSE_ALifeMonsterAbstract* monster = smart_cast<CSE_ALifeMonsterAbstract*>(object);
			if (!monster)
				continue;

			const PATH& path = monster->brain().movement().detail().path();
			const float& walked_distance =
				(path.size() < 2) ? 0.f : monster->brain().movement().detail().walked_distance();
			//			font.OutNext		("%s",monster->name_replace());

			if ((path.size() >= 2) && !fis_zero(walked_distance))
				continue;

			if (!first_time)
				continue;

			fvec3 position = convert_position(graph.vertex(monster->m_tGraphID)->game_point());
			render.draw_aabb(position, radius, radius, radius, color);
			first_time = false;
			continue;
		}
	}

	const_iterator I = objects.objects().begin();
	const_iterator E = objects.objects().end();
	for (; I != E; ++I)
	{
		CSE_ALifeDynamicObject* object = (*I).second;
		CSE_ALifeMonsterAbstract* monster = smart_cast<CSE_ALifeMonsterAbstract*>(object);
		if (!monster)
			continue;

		const PATH& path = monster->brain().movement().detail().path();
		if (path.size() < 2)
			continue;

		u32 game_vertex_id0 = monster->m_tGraphID;
		u32 game_vertex_id1 = path[path.size() - 2];
		const float& walked_distance = monster->brain().movement().detail().walked_distance();

		if (fis_zero(walked_distance))
			continue;

		fvec3 position0 = graph.vertex(game_vertex_id0)->game_point();
		fvec3 position1 = graph.vertex(game_vertex_id1)->game_point();
		const float distance = position0.distance_to(position1);

		position0 = convert_position(position0);
		position1 = convert_position(position1);

		fvec3 direction = fvec3().sub(position1, position0);
		float magnitude = direction.magnitude();
		direction.normalize();
		direction.mul(magnitude * walked_distance / distance);
		direction.add(position0);
		render.draw_aabb(direction, radius, radius, radius, color);

		fvec4 temp;
		Engine.RenderView.ViewProjection.transform(temp, direction);

		if (temp.z < 0.f)
			continue;

		if (temp.w < 0.f)
			continue;

		if (temp.x < -1.f)
			continue;

		if (temp.x > 1.f)
			continue;

		if (temp.y < -1.f)
			continue;

		if (temp.x > 1.f)
			continue;

		font.SetHeightI(.05f / _sqrt(temp.w));
	}
}

void CLevelGraph::draw_game_graph()
{
	if (!Level().CurrentEntity())
		return;

	//	fvec3					camera_position = Level().CurrentEntity()->Position();
	//	CGameFont				*font = HUD().Font().pFontDI;

	const fmat4x4& transform = Level().CurrentEntity()->Transform();
	fvec3 center = fvec3().set(0.f, 5.f, 0.f);
	fvec3 bounds = fvec3().set(3.f, 0.f, 3.f);

	// draw back plane
	fvec3 vertices[4];
	transform.transform_tiny(vertices[0], fvec3().set(center.x - bounds.x, center.y + bounds.y, center.z + bounds.z));
	transform.transform_tiny(vertices[1], fvec3().set(center.x + bounds.x, center.y + bounds.y, center.z + bounds.z));
	transform.transform_tiny(vertices[2], fvec3().set(center.x - bounds.x, center.y - bounds.y, center.z - bounds.z));
	transform.transform_tiny(vertices[3], fvec3().set(center.x + bounds.x, center.y - bounds.y, center.z - bounds.z));

	//	u32						back_color = D3DCOLOR_XRGB(0,0,0);
	//	RenderBackend.dbg_DrawTRI		(Fidentity,vertices[0],vertices[2],vertices[1],back_color);
	//	RenderBackend.dbg_DrawTRI		(Fidentity,vertices[1],vertices[2],vertices[3],back_color);

	// draw vertices
	CGameGraph& graph = ai().game_graph();
	update_current_info();

	bool found = false;
	bool all = (m_current_level_id == -1);
	for (int i = 0, n = (int)graph.header().vertex_count(); i < n; ++i)
	{
		if (!all)
		{
			if (graph.vertex(i)->level_id() != m_current_level_id)
			{
				if (found)
					break;

				continue;
			}

			found = true;
		}

		draw_vertex(i);

		if (psAI_Flags.test(aiDrawGameGraphStalkers))
			draw_stalkers(i);

		if (psAI_Flags.test(aiDrawGameGraphObjects))
			draw_objects(i);
	}

	/**
	for (int i=0; i<(int)ai().game_graph().header().vertex_count(); ++i) {
		fvec3 t1 = ai().game_graph().vertex(i)->game_point();
		t1.y += .6f;
		NORMALIZE_VECTOR(t1);
		Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,255));
		CGameGraph::const_iterator	I, E;
		ai().game_graph().begin		(i,I,E);
		for ( ; I != E; ++I) {
			fvec3 t2 = ai().game_graph().vertex((*I).vertex_id())->game_point();
			t2.y += .6f;
			NORMALIZE_VECTOR(t2);
			Level().debug_renderer().draw_line(Fidentity,t1,t2,D3DCOLOR_XRGB(0,255,0));
		}
		fvec3         T;
		fvec4        S;
		T.set			(t1);
		//T.y+= 1.5f;
		T.y+= 1.5f/10.f;
		Engine.RenderView.ViewProjection.transform (S,T);
		//out of screen
		if (S.z < 0 || S.w < 0)												continue;
		if (S.x < -1.f || S.x > 1.f || S.y<-1.f || S.x>1.f)					continue;
		F->SetSizeI	(0.05f/_sqrt(std::abs(S.w)));
		F->SetColor(0xffffffff);
		F->OutI(S.x,-S.y,"%d",i);
	}
	{
		const xr_vector<u32>	&path = map_point_path;
		if( path.size() ){
			fvec3 t1 = ai().game_graph().vertex(path.back())->game_point();
			t1.y += .6f;
			NORMALIZE_VECTOR(t1);
			Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,255));
			for (int i=(int)path.size() - 2; i>=0;--i) {
				fvec3 t2 = ai().game_graph().vertex(path[i])->game_point();
				t2.y += .6f;
				NORMALIZE_VECTOR(t2);
				Level().debug_renderer().draw_aabb(t2,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,255));
				Level().debug_renderer().draw_line(Fidentity,t1,t2,D3DCOLOR_XRGB(0,0,255));
				t1 = t2;
			}
		}
	}

	if (GameID() == GAME_SINGLE && ai().get_alife()) {
		{
			GameGraph::_LEVEL_ID	J = ai().game_graph().vertex(ai().alife().graph().actor()->m_tGraphID)->level_id();
			for (int i=0, n=(int)ai().game_graph().header().vertex_count(); i<n; ++i) {
				if (ai().game_graph().vertex(i)->level_id() != J)
					continue;
				fvec3 t1 = ai().game_graph().vertex(i)->level_point(), t2 = ai().game_graph().vertex(i)->game_point();
				t1.y += .6f;
				t2.y += .6f;
				NORMALIZE_VECTOR(t2);
				Level().debug_renderer().draw_aabb(t1,.5f,.5f,.5f,D3DCOLOR_XRGB(255,255,255));
				//Level().debug_renderer().draw_line(Fidentity,t1,t2,D3DCOLOR_XRGB(255,255,255));
				fvec3         T;
				fvec4        S;
				T.set			(t1);
				//T.y+= 1.5f;
				T.y+= 1.5f;
				Engine.RenderView.ViewProjection.transform (S,T);
				//out of screen
				if (S.z < 0 || S.w < 0)												continue;
				if (S.x < -1.f || S.x > 1.f || S.y<-1.f || S.x>1.f)					continue;
				F->SetSizeI	(0.1f/_sqrt(std::abs(S.w)));
				F->SetColor(0xffffffff);
				F->OutI(S.x,-S.y,"%d",i);
			}
		}

		ALife::D_OBJECT_P_MAP::const_iterator	I = ai().alife().objects().objects().begin();
		ALife::D_OBJECT_P_MAP::const_iterator	E = ai().alife().objects().objects().end();
		for ( ; I != E; ++I) {
			{
				CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract *>((*I).second);
				if (tpALifeMonsterAbstract && tpALifeMonsterAbstract->m_bDirectControl &&
	!tpALifeMonsterAbstract->m_bOnline) { CSE_ALifeHumanAbstract *tpALifeHuman = smart_cast<CSE_ALifeHumanAbstract
	*>(tpALifeMonsterAbstract); if (tpALifeHuman && tpALifeHuman->brain().movement().detail().path().size()) { fvec3
	t1 = ai().game_graph().vertex(tpALifeHuman->brain().movement().detail().path().back())->game_point(); t1.y += .6f;
						NORMALIZE_VECTOR(t1);
						Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,255));
						for (int i=(int)tpALifeHuman->brain().movement().detail().path().size() - 2; i>=0;--i) {
							fvec3 t2 =
	ai().game_graph().vertex(tpALifeHuman->brain().movement().detail().path()[i])->game_point(); t2.y += .6f;
							NORMALIZE_VECTOR(t2);
							Level().debug_renderer().draw_aabb(t2,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,255));
							Level().debug_renderer().draw_line(Fidentity,t1,t2,D3DCOLOR_XRGB(0,0,255));
							t1 = t2;
						}
					}
					if (tpALifeMonsterAbstract->m_fDistanceToPoint > EPS_L) {
						fvec3 t1 = ai().game_graph().vertex(tpALifeMonsterAbstract->m_tGraphID)->game_point();
						fvec3 t2 = ai().game_graph().vertex(tpALifeMonsterAbstract->m_tNextGraphID)->game_point();
						t2.sub(t1);
						t2.mul(tpALifeMonsterAbstract->m_fDistanceFromPoint/tpALifeMonsterAbstract->m_fDistanceToPoint);
						t1.add(t2);
						t1.y += .6f;
						NORMALIZE_VECTOR(t1);
						Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(255,0,0));
					}
					else {
						fvec3 t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
						t1.y += .6f;
						NORMALIZE_VECTOR(t1);
						Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(255,0,0));
					}
				}
				else {
					CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>((*I).second);
					if (l_tpALifeInventoryItem && !l_tpALifeInventoryItem->attached()) {
						fvec3 t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
						t1.y += .6f;
						NORMALIZE_VECTOR(t1);
						Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(255,255,0));
					}
					else {
						CSE_ALifeCreatureActor *tpALifeCreatureActor = smart_cast<CSE_ALifeCreatureActor*>((*I).second);
						if (tpALifeCreatureActor) {
							fvec3 t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
							t1.y += .6f;
							NORMALIZE_VECTOR(t1);
							Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(255,255,255));
						}
						else {
							CSE_ALifeTrader *tpALifeTrader = smart_cast<CSE_ALifeTrader*>((*I).second);
							if (tpALifeTrader) {
								fvec3 t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
								t1.y += .6f;
								NORMALIZE_VECTOR(t1);
								Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(0,0,0));
							}
							else {
								CSE_ALifeSmartZone *smart_zone = smart_cast<CSE_ALifeSmartZone*>((*I).second);
								if (smart_zone) {
									fvec3 t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
									t1.y += .6f;
									NORMALIZE_VECTOR(t1);
									Level().debug_renderer().draw_aabb(t1,.05f,.05f,.05f,D3DCOLOR_XRGB(255,0,0));
								}
							}
						}
					}
				}
			}
		}
	}
	/**/
}

#endif // AI_COMPILER
#endif // DEBUG
