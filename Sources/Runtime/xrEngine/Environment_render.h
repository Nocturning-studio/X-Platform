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

		// ”станавливаем сэмплеры дл€ текстур скайбокса
		// Ёти сэмплеры прив€заны к рендер-таргетам "$user$sky0" и "$user$sky1"
		// которые заполн€ютс€ в CEnvironment::OnFrame()
		C.set_Sampler("s_sky0", "$user$sky0");
		C.set_Sampler("s_sky1", "$user$sky1");

		C.end_Pass();
	}
};
