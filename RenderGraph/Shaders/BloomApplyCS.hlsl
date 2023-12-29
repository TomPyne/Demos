Texture2D<float4> InTex : register(t0);
RWTexture2D<float4> OutTex : register(u0);

SamplerState BilinearClampedSamp : register(s2);

cbuffer cbuf : register(b0)
{
    uint2 _dims;
    float _strength;
    float _pad;
};

float3 Prefilter(float3 c)
{
    float threshold = 0.2f;

    float brightness = max(c.r, max(c.g, c.b));
    float contribution = max(0.0f, brightness - threshold);

    contribution /= max(0.00001f, brightness);

    return c * contribution;
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if(threadId.x >= _dims.x || threadId.y >= _dims.y)
        return;

    float u = ((float)threadId.x + 0.5f) / (float)_dims.x;
    float v = ((float)threadId.y + 0.5f) / (float)_dims.y;

    float3 bloom = InTex.SampleLevel(BilinearClampedSamp, float2(u,v), 0).rgb;
    float4 color = OutTex[threadId.xy];

    OutTex[threadId.xy] = float4(lerp(color.rgb, bloom, _strength), color.a);
}