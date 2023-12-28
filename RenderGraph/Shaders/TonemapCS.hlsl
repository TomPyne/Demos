
RWTexture2D<float4> Tex : register(u0);

cbuffer cbuf : register(b0)
{
    uint2 _dimension;
    float2 pad;
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if(threadId.x >= _dimension.x || threadId.y >= _dimension.y)
        return;

    float4 srcVal = Tex[threadId.xy];
    float3 x = srcVal.rgb;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    float3 v = saturate((x * (a * x + b)) / (x * (c * x + d) + e));
    Tex[threadId.xy] = float4(v, srcVal.a);
}