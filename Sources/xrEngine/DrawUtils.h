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
	virtual void __stdcall DrawCross(const float3& p, float szx1, float szy1, float szz1, float szx2, float szy2,
									 float szz2, u32 clr, BOOL bRot45 = false) = 0;
	virtual void __stdcall DrawCross(const float3& p, float sz, u32 clr, BOOL bRot45 = false) = 0;
	virtual void __stdcall DrawFlag(const float3& p, float heading, float height, float sz, float sz_fl, u32 clr,
									BOOL bDrawEntity) = 0;
	virtual void __stdcall DrawRomboid(const float3& p, float radius, u32 clr) = 0;
	virtual void __stdcall DrawJoint(const float3& p, float radius, u32 clr) = 0;

	virtual void __stdcall DrawSpotLight(const float3& p, const float3& d, float range, float phi, u32 clr) = 0;
	virtual void __stdcall DrawDirectionalLight(const float3& p, const float3& d, float radius, float range,
												u32 clr) = 0;
	virtual void __stdcall DrawPointLight(const float3& p, float radius, u32 clr) = 0;

	virtual void __stdcall DrawSound(const float3& p, float radius, u32 clr) = 0;
	virtual void __stdcall DrawLineSphere(const float3& p, float radius, u32 clr, BOOL bCross) = 0;

	virtual void __stdcall dbgDrawPlacement(const float3& p, int sz, u32 clr, LPCSTR caption = 0,
											u32 clr_font = 0xffffffff) = 0;
	virtual void __stdcall dbgDrawVert(const float3& p0, u32 clr, LPCSTR caption = 0) = 0;
	virtual void __stdcall dbgDrawEdge(const float3& p0, const float3& p1, u32 clr, LPCSTR caption = 0) = 0;
	virtual void __stdcall dbgDrawFace(const float3& p0, const float3& p1, const float3& p2, u32 clr,
									   LPCSTR caption = 0) = 0;

	virtual void __stdcall DrawFace(const float3& p0, const float3& p1, const float3& p2, u32 clr_s, u32 clr_w,
									BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawLine(const float3& p0, const float3& p1, u32 clr) = 0;
	virtual void __stdcall DrawLine(const float3* p, u32 clr) = 0;
	virtual void __stdcall DrawLink(const float3& p0, const float3& p1, float sz, u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const float3& p0, const float3& p1, const float3& p2, float size,
										  u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const float3* p, float size, u32 clr) = 0;
	virtual void __stdcall DrawFaceNormal(const float3& C, const float3& N, float size, u32 clr) = 0;
	virtual void __stdcall DrawSelectionBox(const float3& center, const float3& size, u32* c = 0) = 0;
	virtual void __stdcall DrawSelectionBox(const Fbox& box, u32* c = 0) = 0;
	virtual void __stdcall DrawIdentSphere(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentCone(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentCylinder(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawIdentBox(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;

	virtual void __stdcall DrawBox(const float3& offs, const float3& Size, BOOL bSolid, BOOL bWire, u32 clr_s,
								   u32 clr_w) = 0;
	virtual void __stdcall DrawAABB(const float3& p0, const float3& p1, u32 clr_s, u32 clr_w, BOOL bSolid,
									BOOL bWire) = 0;
	virtual void __stdcall DrawAABB(const float4x4& parent, const float3& center, const float3& size, u32 clr_s,
									u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawOBB(const float4x4& parent, const Fobb& box, u32 clr_s, u32 clr_w) = 0;
	virtual void __stdcall DrawSphere(const float4x4& parent, const float3& center, float radius, u32 clr_s, u32 clr_w,
									  BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawSphere(const float4x4& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid,
									  BOOL bWire) = 0;
	virtual void __stdcall DrawCylinder(const float4x4& parent, const float3& center, const float3& dir, float height,
										float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawCone(const float4x4& parent, const float3& apex, const float3& dir, float height,
									float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawPlane(const float3& center, const float2& scale, const float3& rotate, u32 clr_s,
									 u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawPlane(const float3& p, const float3& n, const float2& scale, u32 clr_s, u32 clr_w,
									 BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
	virtual void __stdcall DrawRectangle(const float3& o, const float3& u, const float3& v, u32 clr_s, u32 clr_w,
										 BOOL bSolid, BOOL bWire) = 0;

	virtual void __stdcall DrawGrid() = 0;
	virtual void __stdcall DrawPivot(const float3& pos, float sz = 5.f) = 0;
	virtual void __stdcall DrawAxis(const float4x4& T) = 0;
	virtual void __stdcall DrawObjectAxis(const float4x4& T, float sz, BOOL sel) = 0;
	virtual void __stdcall DrawSelectionRect(const int2& m_SelStart, const int2& m_SelEnd) = 0;

	virtual void __stdcall OutText(const float3& pos, LPCSTR text, u32 color = 0xFF000000,
								   u32 shadow_color = 0xFF909090) = 0;
};
//----------------------------------------------------
#endif
