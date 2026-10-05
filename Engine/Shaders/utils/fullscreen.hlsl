////////////////////////////////////////////////////////////////////////////////
// Created: 05.10.2026 15:33:07
// Author: NS_Deathman
// File: fullscreen.hlsl
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
struct VertexInput
{
    float2 Position : POSITION;
    float2 TexCoords : TEXCOORD0;
};

struct Interpolants
{
    float4 Position : POSITION;
    float2 TexCoords : TEXCOORD0;
};

Interpolants vs_main(VertexInput Input)
{
    Interpolants Output;
    Output.Position = float4(Input.Position, 0.0f, 1.0f);
    Output.TexCoords = Input.TexCoords;
    return Output;
}
////////////////////////////////////////////////////////////////////////////////
