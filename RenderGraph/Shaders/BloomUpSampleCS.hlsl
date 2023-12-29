Texture2D<float4> InTex : register(t0);
RWTexture2D<float4> OutTex : register(u0);

SamplerState BilinearClampedSamp : register(s2);

cbuffer cbuf : register(b0)
{
    uint2 _dims;
    float2 _texelSize;
};

float3 Prefilter(float3 c)
{
    float threshold = 0.2f;

    float brightness = max(c.r, max(c.g, c.b));
    float contribution = max(0.0f, brightness - threshold);

    contribution /= max(0.00001f, brightness);

    return c * contribution;
}

float3 BlurSample(const float2 tc)
{
    return (InTex.SampleLevel(BilinearClampedSamp, tc, 0).rgb);
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if(threadId.x >= _dims.x || threadId.y >= _dims.y)
        return;

    float x = _texelSize.x;
    float y = _texelSize.y;

    float u = ((float)threadId.x + 0.5f) / (float)_dims.x;
    float v = ((float)threadId.y + 0.5f) / (float)_dims.y;

    float2 texCoord = float2(u, v);

    float x2 = 2.0f * x;
    float y2 = 2.0f * y;

    float3 a = BlurSample(float2(texCoord.x - x2, texCoord.y + y2)).rgb;
    float3 b = BlurSample(float2(texCoord.x,      texCoord.y + y2)).rgb;
    float3 c = BlurSample(float2(texCoord.x + x2, texCoord.y + y2)).rgb;
                        
    float3 d = BlurSample(float2(texCoord.x - x2, texCoord.y)).rgb;
    float3 e = BlurSample(float2(texCoord.x,      texCoord.y)).rgb;
    float3 f = BlurSample(float2(texCoord.x + x2, texCoord.y)).rgb;

    float3 g = BlurSample(float2(texCoord.x - x2, texCoord.y - y2)).rgb;
    float3 h = BlurSample(float2(texCoord.x,      texCoord.y - y2)).rgb;
    float3 i = BlurSample(float2(texCoord.x + x2, texCoord.y - y2)).rgb;

    float3 j = BlurSample(float2(texCoord.x - x, texCoord.y + y)).rgb;
    float3 k = BlurSample(float2(texCoord.x + x, texCoord.y + y)).rgb;
    float3 l = BlurSample(float2(texCoord.x - x, texCoord.y - y)).rgb;
    float3 m = BlurSample(float2(texCoord.x + x, texCoord.y - y)).rgb;

    float3 downSampled = e * 0.125f;
    downSampled += (a+c+g+i)*0.03125f;
    downSampled += (b+d+f+h)*0.0625f;
    downSampled += (j+k+l+m)*0.125f;
    OutTex[threadId.xy] = float4(downSampled, 1);
}