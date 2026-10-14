#pragma once
#include "blender.h"

class CBlender_skybox : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combiner";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		C.begin_Pass("sky2", "sky2", "main", "main", FALSE, TRUE, FALSE);

		// Устанавливаем сэмплеры для текстур скайбокса
		// Эти сэмплеры привязаны к рендер-таргетам "$user$sky0" и "$user$sky1"
		// которые заполняются в CEnvironment::OnFrame()
		C.set_Sampler("s_sky0", "$user$sky0");
		C.set_Sampler("s_sky1", "$user$sky1");

		C.end_Pass();
	}
};
