#pragma once



class ENGINE_API CEnvModifier
{
  public:
    fvec3 position;
    float radius;
    float power;
    float far_plane;
    fvec3 fog_color;
    float fog_density;
    fvec3 ambient;
    fvec3 sky_color;
    fvec3 hemi_color;
    Flags16 use_flags;

    bool load(IReader* fs);
    float sum(CEnvModifier& _another, fvec3& view);
};