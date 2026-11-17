TextureCube g_EnvironmentMap : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer BakeConstants : register(b0)
{
    float g_Roughness;
    uint g_FaceIndex;
    float g_SourceSize;
    float _padding;
};

static const float PI = 3.14159265359;
static const uint SAMPLE_COUNT = 1024u;

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float3 UVToDirection(float2 uv, uint face)
{
    float2 st = uv * 2.0 - 1.0;
    float3 dir;
    if (face == 0)
        dir = float3(1.0, -st.y, -st.x);
    else if (face == 1)
        dir = float3(-1.0, -st.y, st.x);
    else if (face == 2)
        dir = float3(st.x, 1.0, st.y);
    else if (face == 3)
        dir = float3(st.x, -1.0, -st.y);
    else if (face == 4)
        dir = float3(st.x, -st.y, 1.0);
    else
        dir = float3(-st.x, -st.y, -1.0);
    return normalize(dir);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

void PixarONB(float3 n, out float3 b1, out float3 b2)
{
    float s = n.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (s + n.z);
    float b = n.x * n.y * a;
    b1 = float3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
    b2 = float3(b, s + n.y * n.y * a, -n.y);
}

float3 RotateToNormal(float3 L, float3 N)
{
    float3 t, b;
    PixarONB(N, t, b);
    return normalize(t * L.x + b * L.y + N * L.z);
}

float3 CosineSampleHemisphere(float2 Xi, float3 N)
{
    float cosTheta = sqrt(1.0 - Xi.x);
    float sinTheta = sqrt(Xi.x);
    float phi = 2.0 * PI * Xi.y;
    float3 L = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    return RotateToNormal(L, N);
}

float4 main(VSOutput input) : SV_Target
{
    float3 N = UVToDirection(input.uv, g_FaceIndex);

    float3 irradiance = float3(0, 0, 0);
    float weight = 0.0;

    [loop]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 L = CosineSampleHemisphere(Xi, N);
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            irradiance += g_EnvironmentMap.SampleLevel(g_Sampler, L, 0).rgb * NdotL;
            weight += NdotL;
        }
    }

    irradiance /= max(weight, 1e-5);
    return float4(irradiance, 1.0);
}
