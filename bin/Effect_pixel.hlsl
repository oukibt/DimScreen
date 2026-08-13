Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target
{
    // float4 col = tex.Sample(samp, input.uv);
    // return col;

    float4 col = tex.Sample(samp, input.uv);
    float3 rgb = col.rgb;
    float lum = dot(rgb, float3(0.2126, 0.7152, 0.0722));

    float dim = saturate(1.0 - pow(lum, 1.35) * 0.25);

    float3 gray = lum.xxx;
    float sat = lerp(1.0, 0.82, saturate(lum * 1.15));
    rgb = lerp(gray, rgb, sat);

    return float4(rgb * dim, col.a);
}