#pragma once
#include "../xrEngine/IGame_Persistent.h"

class CHOM
{
private:
    CDB::MODEL * m_pModel;
    BOOL bEnabled;

public:
    void Load();
    void Unload();

    const CDB::MODEL* get_occluder_model() const { return m_pModel; }

    BOOL visible(vis_data& vis);
    BOOL visible(Fbox3& B);
    BOOL visible(sPoly& P);
    BOOL visible(Fbox2& B, float depth);

    BOOL invisible(vis_data& vis) { return !visible(vis); };
    BOOL invisible(Fbox3& B) { return !visible(B); };
    BOOL invisible(sPoly& P) { return !visible(P); };
    BOOL invisible(Fbox2& B, float depth) { return !visible(B, depth); };

    CHOM();
    ~CHOM();
};
