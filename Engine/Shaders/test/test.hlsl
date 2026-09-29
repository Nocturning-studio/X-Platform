struct VS_IN { float3 pos : POSITION; float4 col : COLOR0; };
struct VS_OUT { float4 pos : POSITION; float4 col : COLOR0; };

VS_OUT vs_main(VS_IN i)
{
    VS_OUT o;
    o.pos = float4(i.pos, 1.0f);
    o.col = i.col;
    return o;
}

struct PS_IN { float4 col : COLOR0; };

float4 ps_main(PS_IN i) : COLOR
{
    return i.col;
}
