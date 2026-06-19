#include "stdafx.h"
#include "HOM.h"
#include "../xrEngine/GameFont.h"
#include "CPUOcclusion.h"

float psOSSR = .001f;

CHOM::CHOM() : bEnabled(FALSE), m_pModel(nullptr) {}
CHOM::~CHOM() { Unload(); }

void CHOM::Load()
{
    string_path fName;
    FS.update_path(fName, "$level$", "level.hom");
    if (!FS.exist(fName)) {
        Msg(" WARNING: Occlusion map '%s' not found.", fName);
        return;
    }
    Msg("* Loading HOM: %s", fName);
    IReader* fs = FS.r_open(fName);
    IReader* S = fs->open_chunk(1);

    CDB::Collector CL;
    while (!S->eof()) {
        struct { fvec3 v1, v2, v3; u32 flags; } P;
        S->r(&P, sizeof(P));
        CL.add_face_packed_D(P.v1, P.v2, P.v3, P.flags, 0.01f);
    }

    m_pModel = xr_new<CDB::MODEL>();
    m_pModel->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
    bEnabled = TRUE;
    S->close();
    FS.r_close(fs);
}

void CHOM::Unload()
{
    xr_delete(m_pModel);
    bEnabled = FALSE;
}

void CHOM::Disable() { bEnabled = FALSE; }
void CHOM::Enable() { bEnabled = m_pModel ? TRUE : FALSE; }

BOOL CHOM::visible(Fbox3& B)
{
    if (!bEnabled) return TRUE;
    if (B.contains(Engine.RenderView.Position)) return TRUE;
    return RenderImplementation.CPUOCC.TestAABB(B);
}

BOOL CHOM::visible(Fbox2& B, float depth)
{
    if (!bEnabled) return TRUE;
    return RenderImplementation.CPUOCC.TestRect(B.min.x, B.min.y, B.max.x, B.max.y, depth);
}

BOOL CHOM::visible(sPoly& P)
{
    if (!bEnabled) return TRUE;
    if (P.empty()) return TRUE;
    return RenderImplementation.CPUOCC.TestPolygon(P);
}

BOOL CHOM::visible(vis_data& vis)
{
    if (Engine.TimeManager.GetFrameCount() < vis.hom_frame) return TRUE;
    if (!bEnabled) return TRUE;

    BOOL result = RenderImplementation.CPUOCC.TestAABB(vis.box);
    u32 frame_current = Engine.TimeManager.GetFrameCount();
    vis.hom_tested = frame_current;
    vis.hom_frame = frame_current + (result ? ::Random.randI(10, 25) : 1);
    return result;
}
