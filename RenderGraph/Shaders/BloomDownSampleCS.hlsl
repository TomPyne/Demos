Texture2D<float4> InTex : register(t0);
RWTexture2D<float4> OutTex : register(u0);

SamplerState BilinearClampedSamp : register(s2);

cbuffer cbuf : register(b0)
{
    uint2 _dims;
    float _radius;
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

float3 BlurSample(float2 tc)
{
    return (InTex.SampleLevel(BilinearClampedSamp, tc, 0).rgb);
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if(threadId.x >= _dims.x || threadId.y >= _dims.y)
        return;

    const float filterRadius = _radius;

    float x = filterRadius;
    float y = filterRadius;

    float u = ((float)threadId.x + 0.5f) / (float)_dims.x;
    float v = ((float)threadId.y + 0.5f) / (float)_dims.y;

    float2 texCoord = float2(u, v);

    float vpy = v+y;
    float vmy = v-y;

    float upx = u+x;
    float umx = u-x;

    float3 a = BlurSample(float2(umx, vpy));
    float3 b = BlurSample(float2(u,   vpy));
    float3 c = BlurSample(float2(upx, vpy));

    float3 d = BlurSample(float2(umx, v));
    float3 e = BlurSample(float2(u,   v));
    float3 f = BlurSample(float2(upx, v));

    float3 g = BlurSample(float2(umx, vmy));
    float3 h = BlurSample(float2(u,   vmy));
    float3 i = BlurSample(float2(upx, vmy));

    float3 upSampled = e * 4.0f;
    upSampled += (b+d+f+h)*2.0f;
    upSampled += (a+c+g+i);
    upSampled *= (1.0f / 16.0f);
    OutTex[threadId.xy] = float4(upSampled, 0);
}