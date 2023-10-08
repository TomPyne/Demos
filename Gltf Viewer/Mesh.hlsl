struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

#ifdef _VS

cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
};

cbuffer transformBuf : register(b1)
{
    row_major float4x4 TransformMatrix;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(TransformMatrix, float4(input.pos.xyz, 1.f));

    output.pos = mul( ViewProjectionMatrix, worldPos);
    output.normal = normalize(mul(TransformMatrix, float4(input.normal, 0.0f)));
    return output;
};

#endif

#ifdef _PS

float4 main(PS_INPUT input) : SV_Target0
{
    float3 n = normalize(input.normal);

    float3 l = normalize(float3(0.2, -1, 0.2));

    float ndl = saturate(dot(n, -l));

    return float4((ndl * 0.8 + 0.1).rrr, 1); 
};

#endif