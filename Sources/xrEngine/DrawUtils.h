//----------------------------------------------------
// file: DrawUtils.h
//----------------------------------------------------

#ifndef DrawUtilsH
#define DrawUtilsH
//----------------------------------------------------
// Utilities
//----------------------------------------------------
class ECORE_API CDUInterface
{
  public:
	//----------------------------------------------------
	virtual void __stdcall DrawCross(const fvec3& p, float szx1, float szy1, float szz1, float szx2, float szy2,
									 float szz2, u32 clr, BOOL bRot45 = false) = 0;
	virtual void __stdcall DrawCross(const fvec3& p, float sz, u32 clr, BOOL bRot45 = false) = 0;
	virtual void __stdcall DrawFlag(const fvec3& p, float heading, float height, float sz, float sz_fl, u32 clr,
									BOOL bDrawEntity) = 0;
	virtual void __stdcall DrawRomboid(const fvec3& p, float radius, u32 clr) = 0;
	virtual void __stdcall DrawJoint(const fvec3& p, float radius, u32 clr) = 0;

	virtual void __stdcall DrawSpotLight(const fvec3& p, const fvec3& d, float range, float phi, u32 clr) = 0;
	virtual void __stdcall DrawDirectionalLight(const fvec3& p, const fvec3& d, float radius, float range,
												u32 clr) = 0;
	virtual void __stdcall DrawPointLight(const fvec3& p, float radius, u32 clr) = 0;

	virtual void __stdcall DrawSound(const fvec3& p, float radius, u32 clr) = 0;
	virtual void __stdcall DrawLineSphere(const fvec3& p, float radius, u32 clr, BOOL bCross) = 0;

	virtual void __stdcall dbgDrawPlacement(const fvec3& p, int sz, u32 clr, LPCSTR caption = 0,
											u32 clr_font = 0xffffffff) = 0;
	virtual void __stdcall dbgDrawVert(const fvec3& p0, u32 clr, LPCSTR caption = 0) = 0;
	virtual void __stdcall dbgDrawEdge(const fvec3& p0, const fvec3& p1, u32 clr, LPCSTR caption = 0) = 0;
	virtual void __stdcall dbgDrawFace(const fvec3& p0, const fvec3& p1, const fvec3& p2, u32 clr,
									   LPCSTR caption = 0) = 0;

	virtual void __stdcall DrawFace(const fvec3& p0, const fvec3& p1, const fvec3& p2, u32 clr_s, u32 clr_w,
									BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawLine(const fvec3& p0, const fvec3& p1, u32 clr) = 0;
	virtual void __stdcall DrawLine(const fvec3* p, u32 clr) = 0;
	virtual void __stdcall DrawLink(const fvec3& p0, const fvec3& p1, float sz, u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const fvec3& p0, const fvec3& p1, const fvec3& p2, float size,
										  u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const fvec3* p, float size, u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const fvec3& C, const fvec3& N, float size, u32 clr) = 0;
	virtual void __stdcall DrawSelectionBox(const fvec3& center, const fvec3& size, u32* c = 0) = 0;
	virtual void __stdcall DrawSelectionBox(const Fbox& box, u32* c = 0) = 0;
	virtual void __stdcall DrawIdentSphere(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentCone(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentCylinder(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentBox(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;

	virtual void __stdcall DrawBox(const fvec3& offs, const fvec3& Size, BOOL bSolid, BOOL bWire, u32 clr_s,
								   u32 clr_w) = 0;
	virtual void __stdcall DrawAABB(const fvec3& p0, const fvec3& p1, u32 clr_s, u32 clr_w, BOOL bSolid,
									BOOL bWire) = 0;
	virtual void __stdcall DrawAABB(const fmat4x4& parent, const fvec3& center, const fvec3& size, u32 clr_s,
									u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawOBB(const fmat4x4& parent, const Fobb& box, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawSphere(const fmat4x4& parent, const fvec3& center, float radius, u32 clr_s, u32 clr_w,
									  BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawSphere(const fmat4x4& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid,
									  BOOL bWire) = 0;
	virtual void __stdcall DrawCylinder(const fmat4x4& parent, const fvec3& center, const fvec3& dir, float height,
										float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawCone(const fmat4x4& parent, const fvec3& apex, const fvec3& dir, float height,
									float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawPlane(const fvec3& center, const fvec2& scale, const fvec3& rotate, u32 clr_s,
									 u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawPlane(const fvec3& p, const fvec3& n, const fvec2& scale, u32 clr_s, u32 clr_w,
									 BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawRectangle(const fvec3& o, const fvec3& u, const fvec3& v, u32 clr_s, u32 clr_w,
										 BOOL bSolid, BOOL bWire) = 0;

	virtual void __stdcall DrawGrid() = 0;
	virtual void __stdcall DrawPivot(const fvec3& pos, float sz = 5.f) = 0;
	virtual void __stdcall DrawAxis(const fmat4x4& T) = 0;
	virtual void __stdcall DrawObjectAxis(const fmat4x4& T, float sz, BOOL sel) = 0;
	virtual void __stdcall DrawSelectionRect(const ivec2& m_SelStart, const ivec2& m_SelEnd) = 0;

	virtual void __stdcall OutText(const fvec3& pos, LPCSTR text, u32 color = 0xFF000000,
								   u32 shadow_color = 0xFF909090) = 0;
};
//----------------------------------------------------
#endif
