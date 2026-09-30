// ============================================================================
// Pass 1: flat colored triangle (offscreen)
// ============================================================================

struct VS_INPUT_TRIANGLE
{
    float3 pos : POSITION;
    float4 col : COLOR0;
};

struct VS_OUTPUT_TRIANGLE
{
    float4 pos : POSITION;
    float4 col : COLOR0;
};

VS_OUTPUT_TRIANGLE vs_main(VS_INPUT_TRIANGLE input)
{
    VS_OUTPUT_TRIANGLE o;
    o.pos = float4(input.pos, 1.0f);
    o.col = input.col;
    return o;
}

float4 ps_main(VS_OUTPUT_TRIANGLE input) : COLOR
{
    return input.col;
}

// ============================================================================
// Pass 2: full-screen quad sampling offscreen texture
// ============================================================================

struct VS_INPUT_QUAD
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUTPUT_QUAD
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VS_OUTPUT_QUAD vs_quad(VS_INPUT_QUAD input)
{
    VS_OUTPUT_QUAD o;
    o.pos = float4(input.pos, 1.0f);
    o.uv = input.uv;
    return o;
}

sampler2D s_tex;

float4 ps_texture(VS_OUTPUT_QUAD input) : COLOR
{
    return tex2D(s_tex, input.uv);
}
