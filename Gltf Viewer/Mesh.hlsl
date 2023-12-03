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
    float3 normal : NORMAL;
    float3 tangent : TANGENT; 
    float2 texcoord : TEXCOORD0;
    float3 worldPos : WORLDPOS;
};

cbuffer meshBuf : register(b1)
{
    row_major float4x4 c_transform;

    float4 c_albedoTint;

    float c_metallicFactor;
    float c_roughnessFactor;
    uint c_useAlbedoTex;
    uint c_useNormalTex;

    uint c_useMetallicRoughnessTex;
    uint c_alphaMask;
    float c_alphaCutoff;
    uint __pad;
}

#ifdef _VS

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
    float4 worldPos = mul(c_transform, float4(input.pos.xyz, 1.f));
    
    output.pos = mul( ViewProjectionMatrix, worldPos );
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(c_transform, float4(input.normal, 0.0f))).xyz;
    output.tangent = normalize(mul(c_transform, float4(input.tangent.xyz, 0.0f))).xyz;
    output.texcoord = input.texcoord;
    return output;
};

#endif

#ifdef _PS

SamplerState TrilinearSamp : register(s1);

Texture2D<float4> AlbedoTexture : register(t0);
Texture2D<float4> NormalTexture : register(t1);
Texture2D<float4> MetallicRoughnessTexture : register(t2);

static const float M_PI = 3.14159265359;

float3 F_Schlick(float3 f0, float VdotH)
{
    return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

float3 SpecularBrdf(float3 f0, float alphaRoughness, float specWeight, float vdh, float ndl, float ndv, float ndh)
{
    float3 F = F_Schlick(f0, vdh);
    float Vis = V_GGX(ndl, ndv, alphaRoughness);
    float D = D_GGX(ndh, alphaRoughness);

    return specWeight * F * Vis * D;
}

float3 DiffuseBrdf(float3 f, float3 diffuse)
{
    return (float3(1.0f, 1.0f, 1.0f) - f) * (1.0f - M_PI) * diffuse;
}

float3 Tonemap(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PS_INPUT input, bool frontFace : SV_IsFrontFace ) : SV_Target0
{
    float4 colAlpha = c_albedoTint;

    if(c_useAlbedoTex)
    {
        colAlpha *= AlbedoTexture.Sample(TrilinearSamp, input.texcoord).rgba;
    }

    // TODO this should be a define to save perf, causes extra depth tests in case its needed.
    if( c_alphaMask && colAlpha.a < c_alphaCutoff )
    {
        discard;
    }

    input.normal = frontFace ? input.normal : -input.normal;

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
    }

    normal = normalize(mul(normal, tangentMatrix));

    float3 spec = 0;
    float3 diff = 0;
    
    const float3 n = normalize(normal);
    const float3 v = -normalize(input.worldPos - CamPos);
    const float3 l = -LightDir;
    const float3 h = normalize(l + v);

    const float vdh = saturate(dot(v, h));
    const float ndl = saturate(dot(n, l));
    const float ndv = saturate(dot(n, v));
    const float ndh = saturate(dot(n, h));

    static const float3 black = 0.0f;
    static const float3 f0Dielectric = 0.04f;

    if(ndl > 0 || ndv > 0)
    {
        const float3 f0 = lerp(f0Dielectric, colAlpha.rgb, metallic);
        const float3 adjustedRadiance = LightRadiance * ndl;

        spec += adjustedRadiance * SpecularBrdf(f0, roughness, 1.0f, vdh, ndl, ndv, ndh);
        diff += adjustedRadiance * DiffuseBrdf(f0, lerp(colAlpha.rgb, black, metallic));
    }

    return float4(Tonemap(diff + spec + LightAmbient), colAlpha.a); 
};

#endif