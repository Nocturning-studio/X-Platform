#pragma once
#include "blender.h"

class CBlender_skybox : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combiner";
    }
    
	virtual BOOL canBeDetailed()
    {
        return FALSE;
    }

    virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	virtual void Compile(CBlender_Compile& C)
    {
		C.begin_Pass("sky2", "sky2", "main", "main", FALSE, TRUE, FALSE);
		C.set_Sampler("s_sky0", "$null", false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
	    C.set_Sampler("s_sky1", "$null", false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
        C.set_Sampler_point("s_autoexposure", "$user$autoexposure");
        C.end_Pass();
    }
};