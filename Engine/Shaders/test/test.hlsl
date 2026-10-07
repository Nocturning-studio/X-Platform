////////////////////////////////////////////////////////////////////////////////
// Fullscreen pass test
//
// Pass 1: colored triangle -> offscreen RT
// Pass 2: fullscreen triangle, sampler 's_src' -> back buffer
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Pass 1: colored triangle
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

VS_TRIANGLE_OUT vs_triangle(VS_TRIANGLE_IN i)
{
    VS_TRIANGLE_OUT o;
    o.pos = float4(i.pos, 1.0f);
    o.col = i.col;
    return o;
}

float4 ps_triangle(VS_TRIANGLE_OUT i) : COLOR
{
    return i.col;
}

// ============================================================================
// Pass 2: fullscreen copy
//
// Контракт input layout задаётся DrawFullscreen:
//   float2 pos : POSITION;   // NDC
//   float2 uv  : TEXCOORD0;  // (0,0) — левый-верхний угол RT
//
// Vertex shader должен вернуть float4(pos, 0, 1) как clip-space position
// и пропустить uv в пиксельный шейдер.
// ============================================================================

struct VS_FULLSCREEN_IN
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_FULLSCREEN_OUT
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VS_FULLSCREEN_OUT vs_fullscreen(VS_FULLSCREEN_IN i)
{
    VS_FULLSCREEN_OUT o;
    o.pos = float4(i.pos, 0.0f, 1.0f);
    o.uv = i.uv;
    return o;
}

sampler2D s_src;

float4 ps_fullscreen(VS_FULLSCREEN_OUT i) : COLOR
{
#ifdef USE_COLOR
    return float4(i.uv, 0, 0);
#else
    return tex2D(s_src, i.uv);
#endif
}
