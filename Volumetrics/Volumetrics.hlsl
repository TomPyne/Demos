cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
    float3 CamPos;
    float pad0;
    float3 LightDir;
    float pad1;
    float3 LightRadiance;
    float pad2;
    float3 LightAmbient;
    float3 pad3;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOS;
};

cbuffer meshBuf : register(b1)
{
    row_major float4x4 c_transform;

    float3 c_transmission;
    float c_sigma;
}

#ifdef _VS

struct VS_INPUT
{
    float3 pos : POSITION;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    float4 worldPos = mul(c_transform, float4(input.pos.xyz, 1.f));
    
    output.pos = mul( ViewProjectionMatrix, worldPos );
    output.worldPos = worldPos.xyz;

    return output;
};

#endif

#ifdef _PS

float4 main(PS_INPUT input ) : SV_Target0
{
    const float3 dir = normalize(input.worldPos - CamPos);

    float3 p1 = input.worldPos;
    float t = 0.0f;
    for(; t < 1.0f; t += 0.01f)
    {
        p1 += dir * 0.01f;
        if(length(p1) > 0.5f)
            break;
    }

    float transmission = exp(-t * c_sigma);

    return float4(c_transmission, 1.0f - transmission);    
}

#endif