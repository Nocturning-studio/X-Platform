#include "pch.h"
#include "pure_relcase.h"
#include "xr_object_list.h"
#include "ILevel.h"

pure_relcase::~pure_relcase()
{
	if(g_pLevel)
		g_pLevel->Objects.relcase_unregister(&m_ID);
}
