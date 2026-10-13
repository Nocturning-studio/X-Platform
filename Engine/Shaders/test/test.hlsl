// ============================================================================
// Pass 1: colored triangle (instanced by CPU-side constant)
// ============================================================================

struct VS_TRIANGLE_IN
{
    float3 pos : POSITION;
    float4 col : COLOR0;
};

struct VS_TRIANGLE_OUT
{
    float4 pos : POSITION;
    float4 col : COLOR0;
};

uniform float4x4 mWorld;

VS_TRIANGLE_OUT vs_triangle(VS_TRIANGLE_IN i)
{
    VS_TRIANGLE_OUT o;
    o.pos = mul(float4(i.pos, 1.0f), mWorld);
    o.col = i.col;
    return o;
}

float4 ps_triangle(VS_TRIANGLE_OUT i) : COLOR
{
    return i.col;
}

// ============================================================================
// Pass 2: fullscreen copy + depth visualization
// ============================================================================

struct VS_FULLSCREEN_IN
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_FULLSCREEN_OUT
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};

VS_FULLSCREEN_OUT vs_fullscreen(VS_FULLSCREEN_IN i)
{
    VS_FULLSCREEN_OUT o;
    o.pos = float4(i.pos, 0.0f, 1.0f);
    o.uv = i.uv;
    return o;
}

sampler2D s_src; // color offscreen
sampler2D s_depth; // INTZ / DF24 / DF16

float4 ps_fullscreen(VS_FULLSCREEN_OUT i) : COLOR
{
    if (i.uv.x < 0.5f)
    {
        float2 uv = float2(i.uv.x * 2.0f, i.uv.y);
        return tex2D(s_src, uv);
    }

    float2 uv = float2((i.uv.x - 0.5f) * 2.0f, i.uv.y);
    float d = tex2D(s_depth, uv).r;
    return float4(d, d, d, 1.0f);
}
