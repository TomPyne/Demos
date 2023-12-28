struct FullscreenInterpolants
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

cbuffer cbuf : register(b0)
{
    float2 _offset;
    float2 _scale;
    float2 _uvOffset;
    float2 _uvScale;
};

#ifdef _VS

static const float2 Verts[6] = 
{
    float2(0, 0), float2(1, 0), float2(1, 1),
    float2(1, 1), float2(0, 1), float2(0, 0)
};

void main(uint vertexID : SV_VertexID, out FullscreenInterpolants Interpolants)
{
    const float2 vert = Verts[vertexID];

    float2 position = vert * _scale + _offset;
    float2 uv = vert *_uvScale + _uvOffset;

    position = position * 2.0f - float2(1.0f, 1.0f);

    Interpolants.Position = float4(position, 0.5, 1);
    Interpolants.UV = uv;
}

#endif

#ifdef _PS

Texture2D<float4> Tex : register(t0);
SamplerState Samp : register(s1);

float4 main(FullscreenInterpolants IN) : SV_TARGET
{
    float3 srcVal = Tex.Sample(Samp, IN.UV).rgb;

    return float4(srcVal, 1.0f);
}
#endif
