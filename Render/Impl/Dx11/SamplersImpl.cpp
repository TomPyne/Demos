#include "../../Samplers.h"

#include "RenderImpl.h"

std::vector<ComPtr<ID3D11SamplerState>> g_Samplers;

static D3D11_TEXTURE_ADDRESS_MODE Dx11_AdressMode(SamplerAddressMode am)
{
	switch (am)
	{
	case SamplerAddressMode::Wrap:
		return D3D11_TEXTURE_ADDRESS_WRAP;
	case SamplerAddressMode::Mirror:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
	case SamplerAddressMode::Clamp:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	case SamplerAddressMode::Border:
		return D3D11_TEXTURE_ADDRESS_BORDER;
	case SamplerAddressMode::MirrorOnce:
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
	}

	assert(0 && "Dx11_AdressMode unsupported");
	return (D3D11_TEXTURE_ADDRESS_MODE)0;
}

static D3D11_COMPARISON_FUNC Dx11_ComparisonFunc(SamplerComparisonFunc cf)
{
	switch (cf)
	{
	case SamplerComparisonFunc::None:
		return (D3D11_COMPARISON_FUNC)0;
	case SamplerComparisonFunc::Never:
		return D3D11_COMPARISON_NEVER;
	case SamplerComparisonFunc::Less:
		return D3D11_COMPARISON_LESS;
	case SamplerComparisonFunc::Equal:
		return D3D11_COMPARISON_EQUAL;
	case SamplerComparisonFunc::LessEqual:
		return D3D11_COMPARISON_LESS_EQUAL;
	case SamplerComparisonFunc::Greater:
		return D3D11_COMPARISON_GREATER;
	case SamplerComparisonFunc::NotEqual:
		return D3D11_COMPARISON_NOT_EQUAL;
	case SamplerComparisonFunc::GreaterEqual:
		return D3D11_COMPARISON_GREATER_EQUAL;
	case SamplerComparisonFunc::Always:
		return D3D11_COMPARISON_ALWAYS;
	}

	assert(0 && "Dx11_ComparisonFunc unsupported");
	return (D3D11_COMPARISON_FUNC)0;
}

static D3D11_FILTER_TYPE Dx11_FilterType(SamplerFilterMode fm)
{
	switch (fm)
	{
	case SamplerFilterMode::Point:
		return D3D11_FILTER_TYPE_POINT;
	case SamplerFilterMode::Linear:
		return D3D11_FILTER_TYPE_LINEAR;
	}

	assert(0 && "Dx11_FilterType was likely passed an aniso type, this should get handled before calling this");
	return (D3D11_FILTER_TYPE)0;
}

static D3D11_FILTER Dx11_Filter(bool comparison, SamplerFilterMode min, SamplerFilterMode mag, SamplerFilterMode mip)
{
	const D3D11_FILTER_REDUCTION_TYPE dxReduction = comparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;
	if (min == SamplerFilterMode::Anisotropic || mag == SamplerFilterMode::Anisotropic || mip == SamplerFilterMode::Anisotropic)
	{
		assert(min == SamplerFilterMode::Anisotropic && mag == SamplerFilterMode::Anisotropic && mip == SamplerFilterMode::Anisotropic && "Dx11_Filter if one filter is aniso, they all must be");
		return D3D11_ENCODE_ANISOTROPIC_FILTER(dxReduction);
	}	

	const D3D11_FILTER_TYPE dxMin = Dx11_FilterType(min);
	const D3D11_FILTER_TYPE dxMag = Dx11_FilterType(min);
	const D3D11_FILTER_TYPE dxMip = Dx11_FilterType(min);

	return D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, dxReduction);
}

static void CreateSampler(size_t index, const SamplerDesc& desc)
{
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = Dx11_Filter(desc.comparison != SamplerComparisonFunc::None, desc.Min, desc.Mag, desc.Mip);
	sd.AddressU = Dx11_AdressMode(desc.AddressU);
	sd.AddressV = Dx11_AdressMode(desc.AddressV);
	sd.AddressW = Dx11_AdressMode(desc.AddressW);
	sd.MipLODBias = desc.mipLODBias;
	sd.MaxAnisotropy = desc.maxAnisotropy;
	sd.ComparisonFunc = Dx11_ComparisonFunc(desc.comparison);
	memcpy(sd.BorderColor, desc.borderColor, sizeof(float) * 4);
	sd.MinLOD = desc.minLOD;
	sd.MaxLOD = desc.maxLOD;

	if (FAILED(g_render.device->CreateSamplerState(&sd, &g_Samplers[index])))
		assert(0 && "CreateSampler failed");
}

void InitSamplers(const SamplerDesc* const descs, size_t count)
{
	g_Samplers.clear();
	g_Samplers.resize(count);

	for (size_t i = 0; i < count; i++)
		CreateSampler(i, descs[i]);
}

size_t Dx11_GetSamplerCount()
{
	return g_Samplers.size();
}

ID3D11SamplerState* Dx11_GetSampler(size_t index)
{
	return g_Samplers[index].Get();
}
