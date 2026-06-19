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
    void Disable();
    void Enable();

    const CDB::MODEL* get_occluder_model() const { return m_pModel; }

    BOOL visible(vis_data& vis);
    BOOL visible(Fbox3& B);
    BOOL visible(sPoly& P);
    BOOL visible(Fbox2& B, float depth);

    CHOM();
    ~CHOM();
};
