////////////////////////////////////////////////////////////////////////////////
// Created: 15.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
////////////////////////////////////////////////////////////////////////////////
SoftX::VertexOutput LightVolumeQueryVS(const SoftX::VertexInput& input,
                                       const SoftX::ConstantBuffer& cb)
{
    const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
    float4 pos = float4(input.Position.x, input.Position.y, input.Position.z, 1.0f);

    float4 clip;
    clip.x = pos.x * mvp._11 + pos.y * mvp._21 + pos.z * mvp._31 + mvp._41;
    clip.y = pos.x * mvp._12 + pos.y * mvp._22 + pos.z * mvp._32 + mvp._42;
    clip.z = pos.x * mvp._13 + pos.y * mvp._23 + pos.z * mvp._33 + mvp._43;
    clip.w = pos.x * mvp._14 + pos.y * mvp._24 + pos.z * mvp._34 + mvp._44;

    SoftX::VertexOutput output;
    output.Position = clip;
    output.Color = float4(0, 0, 0, 0);
    output.Normal = float3(0, 0, 0);
    output.UV = float2(0, 0);
    return output;
}

SoftX::VertexOutput LightVolumeDebugVS(
    const SoftX::VertexInput& input,
    const SoftX::ConstantBuffer& cb,
    const SoftX::TextureTable& /*tex*/)
{
    const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
    float4 pos = float4(input.Position.x, input.Position.y, input.Position.z, 1.0f);

    float4 clip;
    clip.x = pos.x * mvp._11 + pos.y * mvp._21 + pos.z * mvp._31 + mvp._41;
    clip.y = pos.x * mvp._12 + pos.y * mvp._22 + pos.z * mvp._32 + mvp._42;
    clip.z = pos.x * mvp._13 + pos.y * mvp._23 + pos.z * mvp._33 + mvp._43;
    clip.w = pos.x * mvp._14 + pos.y * mvp._24 + pos.z * mvp._34 + mvp._44;

    SoftX::VertexOutput output;
    output.Position = clip;
    output.Color = float4(0, 0, 0, 0);
    output.Normal = float3(0, 0, 0);
    output.UV = float2(0, 0);
    return output;
}
////////////////////////////////////////////////////////////////////////////////
