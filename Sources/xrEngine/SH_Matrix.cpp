#include "stdafx.h"
#pragma hdrstop

void CMatrix::Calculate()
{
	if (dwFrame == Engine.TimeManager.GetFrameCount())
		return;
	dwFrame = Engine.TimeManager.GetFrameCount();

	// Switch on mode
	switch (dwMode)
	{
	case modeProgrammable:
	case modeDetail:
		return;
	case modeTCM: {
		fmat4x4 T;
		float sU = 1, sV = 1, t = Engine.TimeManager.GetGlobalTime();
		tc_trans(transform, .5f, .5f);
		if (tcm & tcmRotate)
		{
			T.rotateZ(rotate.Calculate(t) * t);
			transform.mulA_43(T);
		}
		if (tcm & tcmScale)
		{
			sU = scaleU.Calculate(t);
			sV = scaleV.Calculate(t);
			T.scale(sU, sV, 1);
			transform.mulA_43(T);
		}
		if (tcm & tcmScroll)
		{
			float u = scrollU.Calculate(t) * t;
			float v = scrollV.Calculate(t) * t;
			u *= sU;
			v *= sV;
			tc_trans(T, u, v);
			transform.mulA_43(T);
		}
		tc_trans(T, -0.5f, -0.5f);
		transform.mulB_43(T);
	}
		return;
	case modeS_refl: {
		float Ux = .5f * Engine.RenderView.View._11, Uy = .5f * Engine.RenderView.View._21, Uz = .5f * Engine.RenderView.View._31, Uw = .5f;
		float Vx = -.5f * Engine.RenderView.View._12, Vy = -.5f * Engine.RenderView.View._22, Vz = -.5f * Engine.RenderView.View._32, Vw = .5f;

		transform._11 = Ux;
		transform._12 = Vx;
		transform._13 = 0;
		transform._14 = 0;
		transform._21 = Uy;
		transform._22 = Vy;
		transform._23 = 0;
		transform._24 = 0;
		transform._31 = Uz;
		transform._32 = Vz;
		transform._33 = 0;
		transform._34 = 0;
		transform._41 = Uw;
		transform._42 = Vw;
		transform._43 = 0;
		transform._44 = 0;
	}
		return;
	case modeC_refl: {
		fmat4x4 M = Engine.RenderView.View;
		M._41 = 0.f;
		M._42 = 0.f;
		M._43 = 0.f;
		transform.invert(M);
	}
		return;
	default:
		return;
	}
}

void CMatrix::Load(IReader* fs)
{
	dwMode = fs->r_u32();
	tcm = fs->r_u32();
	fs->r(&scaleU, sizeof(WaveForm));
	fs->r(&scaleV, sizeof(WaveForm));
	fs->r(&rotate, sizeof(WaveForm));
	fs->r(&scrollU, sizeof(WaveForm));
	fs->r(&scrollV, sizeof(WaveForm));
}

void CMatrix::Save(IWriter* fs)
{
	fs->w_u32(dwMode);
	fs->w_u32(tcm);
	fs->w(&scaleU, sizeof(WaveForm));
	fs->w(&scaleV, sizeof(WaveForm));
	fs->w(&rotate, sizeof(WaveForm));
	fs->w(&scrollU, sizeof(WaveForm));
	fs->w(&scrollV, sizeof(WaveForm));
}
