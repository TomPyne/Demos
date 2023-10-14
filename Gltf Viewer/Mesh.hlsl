cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
    float3 CamPos;
    float pad0;
    float3 LightDir;
    float pad1;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT; // W component is handedness
    float2 texcoord : TEXCOORD0;
};

#ifdef _VS

cbuffer transformBuf : register(b1)
{
    row_major float4x4 TransformMatrix;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(TransformMatrix, float4(input.pos.xyz, 1.f));

    output.pos = mul( ViewProjectionMatrix, worldPos);
    output.normal = normalize(mul(TransformMatrix, float4(input.normal, 0.0f))).xyz;
    output.tangent = normalize(mul(TransformMatrix, float4(input.tangent.xyz, 0.0f))).xyz;
    output.texcoord = input.texcoord;
    return output;
};

#endif

#ifdef _PS

cbuffer MaterialBuf : register(b1)
{
    float4 c_albedoTint;

    float c_metallicFactor;
    float c_roughnessFactor;
    uint c_useAlbedoTex;
    uint c_useNormalTex;

    uint c_useMetallicRoughnessTex;
    uint3 __pad;
};

SamplerState TrilinearSamp : register(s1);

Texture2D<float4> AlbedoTexture : register(t0);
Texture2D<float4> NormalTexture : register(t1);
Texture2D<float4> MetallicRoughnessTexture : register(t2);

float4 main(PS_INPUT input) : SV_Target0
{
    float4 col = c_albedoTint;

    if(c_useAlbedoTex)
    {
        col *= AlbedoTexture.Sample(TrilinearSamp, input.texcoord).rgba;
    }

    input.tangent = normalize(input.tangent - dot(input.tangent, input.normal)*input.normal);
    float3 bitangent = cross(input.normal, input.tangent);

    float3x3 tangentMatrix = float3x3(input.tangent,bitangent, input.normal);
    
    float3 normal = float3(0, 0, 1);
    if(c_useNormalTex)
    {
        normal = NormalTexture.Sample(TrilinearSamp, input.texcoord).rgb;
        normal = (2.0f * normal) - float(1.0f).rrr;
    }

    float metallic = 0.0f;
    float roughness = c_roughnessFactor;

    if(c_useMetallicRoughnessTex)
    {
        float2 metallicRoughnessSample = MetallicRoughnessTexture.Sample(TrilinearSamp, input.texcoord).rg;
        metallic = metallicRoughnessSample.x * c_metallicFactor;
        roughness = metallicRoughnessSample.y * c_roughnessFactor;

        col.rgb = float3(metallicRoughnessSample, 0);
    }

    normal = normalize(mul(normal, tangentMatrix));
    
    float3 n = normalize(normal);

    float ndl = saturate(dot(n, -LightDir));

    return float4(ndl * col.rgb + (0.1 * col.rgb), 1); 
};

#endif