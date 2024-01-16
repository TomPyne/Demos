cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
    float3 CamPos;
    float TotalTime;
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

    float c_sigma_s;
    float c_sigma_a;
    float c_asymmetry;
    float c_noiseScale;

    float3 c_panDir;
    float c_density;

    float c_stepSize;
    float3 pad;
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

/////////////////////////////////////////////////////////////////////////////////////////////////
// NOISE
/////////////////////////////////////////////////////////////////////////////////////////////////

/* https://www.shadertoy.com/view/XsX3zB
 *
 * The MIT License
 * Copyright © 2013 Nikita Miropolskiy
 * 
 * ( license has been changed from CCA-NC-SA 3.0 to MIT
 *
 *   but thanks for attributing your source code when deriving from this sample 
 *   with a following link: https://www.shadertoy.com/view/XsX3zB )
 *
 * ~
 * ~ if you're looking for procedural noise implementation examples you might 
 * ~ also want to look at the following shaders:
 * ~ 
 * ~ Noise Lab shader by candycat: https://www.shadertoy.com/view/4sc3z2
 * ~
 * ~ Noise shaders by iq:
 * ~     Value    Noise 2D, Derivatives: https://www.shadertoy.com/view/4dXBRH
 * ~     Gradient Noise 2D, Derivatives: https://www.shadertoy.com/view/XdXBRH
 * ~     Value    Noise 3D, Derivatives: https://www.shadertoy.com/view/XsXfRH
 * ~     Gradient Noise 3D, Derivatives: https://www.shadertoy.com/view/4dffRH
 * ~     Value    Noise 2D             : https://www.shadertoy.com/view/lsf3WH
 * ~     Value    Noise 3D             : https://www.shadertoy.com/view/4sfGzS
 * ~     Gradient Noise 2D             : https://www.shadertoy.com/view/XdXGW8
 * ~     Gradient Noise 3D             : https://www.shadertoy.com/view/Xsl3Dl
 * ~     Simplex  Noise 2D             : https://www.shadertoy.com/view/Msf3WH
 * ~     Voronoise: https://www.shadertoy.com/view/Xd23Dh
 * ~ 
 *
 */

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
float3 random3(float3 c) {
	float j = 4096.0*sin(dot(c,float3(17.0, 59.4, 15.0)));
	float3 r;
	r.z = frac(512.0*j);
	j *= .125;
	r.x = frac(512.0*j);
	j *= .125;
	r.y = frac(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
static const float F3 =  0.3333333;
static const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(float3 p) {
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 float3 s = floor(p + dot(p, F3.rrr));
	 float3 x = p - s + dot(s, G3.rrr);
	 
	 /* calculate i1 and i2 */
	 float3 e = step(float3(0, 0, 0), x - x.yzx);
	 float3 i1 = e*(1.0 - e.zxy);
	 float3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 float3 x1 = x - i1 + G3;
	 float3 x2 = x - i2 + 2.0*G3;
	 float3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 float4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, float(52.0).rrrr);
}

/* const matrices for 3d rotation */
static const float3x3 rot1 = float3x3(-0.37, 0.36, 0.85,-0.14,-0.93, 0.34,0.92, 0.01,0.4);
static const float3x3 rot2 = float3x3(-0.55,-0.39, 0.74, 0.33,-0.91,-0.24,0.77, 0.12,0.63);
static const float3x3 rot3 = float3x3(-0.71, 0.52,-0.47,-0.08,-0.72,-0.68,-0.7,-0.45,0.56);

/* directional artifacts can be reduced by rotating each octave */
float simplex3d_fractal(float3 m) {
    return   0.5333333*simplex3d(mul(m, rot1))
			+0.2666667*simplex3d(mul(mul(2.0,m), rot2))
			+0.1333333*simplex3d(mul(mul(4.0,m),rot3))
			+0.0666667*simplex3d(mul(8.0,m));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// /NOISE
/////////////////////////////////////////////////////////////////////////////////////////////////

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

static const float PI = 3.14159265359f;

float IsotropicPhase()
{
    return 1.0f / (4.0f * PI);
}

float HenyeyGreensteinPhase(float g, float cosTheta)
{
    float gg = g * g;
    float n = 1.0f - gg;
    float d = pow(1.0f + gg - 2.0f * g * cosTheta, 3.0f / 2.0f);

    return IsotropicPhase() * (n / d);
}

float Rand(float x)
{
    return frac(sin(x) * 100000.0f);
}

float4 main(PS_INPUT input ) : SV_Target0
{
    float3 startPos = input.objPos;
    float3 eyeDir = normalize(input.worldPos - CamPos);

    startPos -= eyeDir * 2.0f;

    Ray eyeRay = MakeRay(startPos, eyeDir);
    float eyet0, eyet1;

    // Generate the intersection points, alpha returns 0 on a miss so we can
    // blend with the final alpha to mask out missed rays.
    float alpha = SphereIntersect(eyeRay, eyet0, eyet1);

    float transparency = 1.0f;

    // Calculate steps required based on the intersection distance
    float stepSize = max(c_stepSize, 0.01f);
    float ns = ceil((eyet1 - eyet0) / stepSize);
    stepSize = (eyet1 - eyet0) / ns;
    float3 result = float3(0, 0, 0);

    // can precalculate phase for directional light since directions remain constant along ray.
    // Wouldn't be possible along a punctual light source.
    const float cosTheta = dot( eyeRay.direction, -LightDir);
    const float phase = HenyeyGreensteinPhase(c_asymmetry, cosTheta);

    // Transmission is a function of the total absorbed and scattered light
    float sigma_t = c_sigma_a + c_sigma_s;

    for(float n = 0.0f; n < ns; n += 1.0f)
    {
        // Jitter the sample locations to prevent banding.
        float t = eyet0 + stepSize * (n + Rand(n + startPos.x + TotalTime));
        float3 samplePos = eyeRay.origin + t * eyeRay.direction;

        float density = max(simplex3d_fractal((samplePos * c_noiseScale) +  c_panDir * TotalTime ), 0) * c_density;

        // Density falloff as we approach edge of sphere.
        density *= 1.0f - length(samplePos * 2.0f);

        // Apply beers law to sample
        float sample_atten = exp(-stepSize * density * sigma_t);

        // Attenuate final transparency.
        transparency *= sample_atten;

        // Calculate in-scatter by tracing distance to edge of sphere.
        // In-scattering assumes homogeneous volume to avoid nested loop.
        // TODO: Calculate lighting for heterogenous volume for self shadowing
        float ist0, ist1;
        if(SphereIntersect(MakeRay(samplePos, -LightDir), ist0, ist1) /*&& ist0 == 0.0f*/)
        {
            // Apply beers law to lighting
            float lightAtten = exp(-density * ist1 * sigma_t);       

            result += density * c_sigma_s * phase * LightRadiance * lightAtten * transparency;
        }
    }

    // Result is multiplied by stepSize so that it is renormalised after we accumulate in the integration
    result *= stepSize;

    // Use regular blending to composite with background
    // In a fully pathtraced pipeline we would lerp the result with the background colour but i let the
    // raster pipeline do this for me by inverting the transparency value    
    return float4(result * stepSize, (1.0f - transparency) * alpha);    
}

#endif