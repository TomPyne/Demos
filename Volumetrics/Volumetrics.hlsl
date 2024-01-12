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
    float3 objPos : OBJPOS;
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
    output.objPos = input.pos.xyz;

    return output;
};

#endif

#ifdef _PS

struct Ray
{
    float3 origin;
    float3 direction;
};

Ray MakeRay(float3 origin, float3 dir)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = dir;
    return ray;
}

static const float SphereRadius = 0.5f;
static const float3 SphereOrigin = float3(0.0f, 0.0f, 0.0f);

float SphereIntersect(Ray ray, out float t0, out float t1)
{
    float3 L = ray.origin - SphereOrigin;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0f * dot(ray.direction, L);
    float c = dot(L, L) - (SphereRadius * SphereRadius);

    float alpha = 0.0f;

    t0 = t1 = 0.0f;

    float d = b * b - 4.0f * a * c;

    if(d == 0.0f)
    {
        t0 = t1 = -0.5f * b / a;

        alpha = 1.0f;
    }
    else if(d > 0.0f)
    {
        float drt = sqrt(d);
        float q = -0.5f * (b > 0 ? b + drt : b - drt);
        t0 = q / a;
        t1 = c / q; 

        alpha = 1.0f;
    }

    if(t0 > t1) 
    {
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    return alpha;
}

float4 main(PS_INPUT input ) : SV_Target0
{
    Ray eyeRay = MakeRay(input.objPos, normalize(input.worldPos - CamPos));
    float eyet0, eyet1;
    float alpha = SphereIntersect(eyeRay, eyet0, eyet1);

    float transparency = 1.0f;

    float stepSize = 0.01f;
    float ns = ceil((eyet1 - eyet0) / stepSize);
    float3 result = float3(0, 0, 0);

    float3 lightCol = float3(1.3f, 0.3f, 0.9f);

    for(float n = 0.0f; n < ns; n += 1.0f)
    {
        float t = eyet1 - stepSize * (n + 0.5f);
        float3 samplePos = eyeRay.origin + t * eyeRay.direction;

        // Apply beers law to sample
        float sampleTransparency = exp(-stepSize * c_sigma);

        // Attenuate final transparency.
        transparency *= sampleTransparency;

        // Calculate in-scatter.
        float ist0, ist1;
        if(SphereIntersect(MakeRay(samplePos, -LightDir), ist0, ist1))
        {
            float lightAtten = exp(-ist1 * c_sigma);
            result += LightRadiance * lightAtten * stepSize;
        }

        result *= sampleTransparency;
    }

    return float4(result, (1.0f - transparency) * alpha);    
}

#endif