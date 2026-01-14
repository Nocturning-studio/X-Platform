#include "StdAfx.h"
#include "light.h"

smapvis::smapvis()
{
	invalidate();
	frame_sleep = 0;
}
smapvis::~smapvis()
{
	flushoccq();
	invalidate();
}
void smapvis::invalidate()
{
	state = state_counting;
	frame_sleep = Engine.TimeManager.GetFrameCount() + ps_r_LightSleepFrames;
	invisible.clear();
}
void smapvis::begin()
{
	RenderImplementation.SceneGraph.clear_Counters();
	switch (state)
	{
	case state_counting:
		// do nothing -> we just prepare for testing process
		break;
	case state_working:
		// mark already known to be invisible visuals, set breakpoint
		testQ_V = 0;
		testQ_id = 0;
		mark();
		RenderImplementation.SceneGraph.set_Feedback(this, test_current);
		break;
	case state_usingTC:
		// just mark
		mark();
		break;
	}
}
void smapvis::end()
{
	// Gather stats
	u32 ts, td;
	RenderImplementation.SceneGraph.get_Counters(ts, td);
	RenderImplementation.stats.ic_total += ts;
	RenderImplementation.SceneGraph.set_Feedback(0, 0);

	switch (state)
	{
	case state_counting:
		// switch to 'working'
		if (sleep())
		{
			test_count = ts;
			test_current = 0;
			state = state_working;
		}
		break;
	case state_working:
		// feedback should be called at this time -> clear feedback
		// issue query
		if (testQ_V)
		{
			RenderImplementation.occq_begin(testQ_id);
			RenderImplementation.SceneGraph.marker += 1;
			RenderImplementation.SceneGraph.r_dsgraph_insert_static(testQ_V);
			RenderImplementation.SceneGraph.r_dsgraph_render_graph(0);
			RenderImplementation.occq_end(testQ_id);
			testQ_frame = Engine.TimeManager.GetFrameCount() + 1; // get result on next frame
		}
		break;
	case state_usingTC:
		// nothing to do
		break;
	}
}

void smapvis::flushoccq()
{
	if (testQ_frame != Engine.TimeManager.GetFrameCount())
		return;

	// Проверка валидности query
	if (testQ_id >= RenderImplementation.HWOCC.GetQuerySize() || testQ_id == 0xffffffff ||
		RenderImplementation.HWOCC.GetUsedQueryByID(testQ_id) == nullptr)
	{
		Msg("! smapvis::flushoccq: Invalid query ID [%u]", testQ_id);
		testQ_V = nullptr;
		return;
	}

	try
	{
		u32 fragments = RenderImplementation.occq_get(testQ_id);
		if (0 == fragments)
		{
			if (testQ_V && std::find(invisible.begin(), invisible.end(), testQ_V) == invisible.end())
				invisible.push_back(testQ_V);
			if (test_count > 0)
				test_count--;
		}
		else
		{
			test_current++;
		}
	}
	catch (...)
	{
		Msg("! smapvis::flushoccq: Exception during occq_get");
	}

	testQ_V = nullptr;

	if (test_current >= test_count && state == state_working)
	{
		state = state_usingTC;
	}
}

void smapvis::resetoccq()
{
	if (testQ_frame == (Engine.TimeManager.GetFrameCount() + 1))
		testQ_frame--;
	flushoccq();
}

void smapvis::mark()
{
	RenderImplementation.stats.ic_culled += invisible.size();
	u32 marker = RenderImplementation.SceneGraph.marker + 1; // we are called befor marker increment
	for (u32 it = 0; it < invisible.size(); it++)
		invisible[it]->vis.marker = marker; // this effectively disables processing
}

void smapvis::rfeedback_static(IRender_Visual* V)
{
	testQ_V = V;
	RenderImplementation.SceneGraph.set_Feedback(0, 0);
}
