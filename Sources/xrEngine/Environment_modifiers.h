#pragma once



class ENGINE_API CEnvModifier
{
  public:
    float3 position;
    float radius;
    float power;
    float far_plane;
    float3 fog_color;
    float fog_density;
    float3 ambient;
    float3 sky_color;
    float3 hemi_color;
    Flags16 use_flags;

    bool load(IReader* fs);
    float sum(CEnvModifier& _another, float3& view);
};