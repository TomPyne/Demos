#include "../BindingImpl.h"

#include "RenderImpl.h"
#include "../../Textures.h"

#include <vector>

std::vector<ComPtr<ID3D11ShaderResourceView>> g_SRVs;
std::vector<ComPtr<ID3D11UnorderedAccessView>> g_UAVs;
std::vector<ComPtr<ID3D11RenderTargetView>> g_RTVs;
std::vector<ComPtr<ID3D11DepthStencilView>> g_DSVs;

static ComPtr<ID3D11ShaderResourceView>& AllocSRV(ShaderResourceView_t srv)
{
	if ((uint32_t)srv >= g_SRVs.size())
		g_SRVs.resize((uint32_t)srv + 1);

	return g_SRVs[(uint32_t)srv];
}

static ComPtr<ID3D11UnorderedAccessView>& AllocUAV(UnorderedAccessView_t uav)
{
	if ((uint32_t)uav >= g_UAVs.size())
		g_UAVs.resize((uint32_t)uav + 1);

	return g_UAVs[(uint32_t)uav];
}

static ComPtr<ID3D11RenderTargetView>& AllocRTV(RenderTargetView_t rtv)
{
	if ((uint32_t)rtv >= g_RTVs.size())
		g_RTVs.resize((uint32_t)rtv + 1);

	return g_RTVs[(uint32_t)rtv];
}

static ComPtr<ID3D11DepthStencilView>& AllocDSV(DepthStencilView_t dsv)
{
	if ((uint32_t)dsv >= g_DSVs.size())
		g_DSVs.resize((uint32_t)dsv + 1);

	return g_DSVs[(uint32_t)dsv];
}

bool CreateTextureSRVImpl(ShaderResourceView_t srv, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t arraySize)
{
	auto& dxSRV = AllocSRV(srv);

	ID3D11Resource* res = Dx11_GetTexture(tex);
	if (!res)
		return true;

	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};

	desc.Format = Dx11_Format(format);
	if (dim == TextureDimension::Tex1D)
	{
		if (arraySize > 1)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
			desc.Texture1DArray.ArraySize = (UINT)arraySize;
			desc.Texture1DArray.MipLevels = (UINT)-1;
		}
		else
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
			desc.Texture1D.MipLevels = (UINT)-1;
		}		
		
	}
	else if (dim == TextureDimension::Tex2D)
	{
		if (arraySize > 1)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = (UINT)arraySize;
			desc.Texture2DArray.MipLevels = (UINT)-1;
		}
		else
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = (UINT)-1;
		}

	}
	else if (dim == TextureDimension::Tex3D)
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		desc.Texture2D.MipLevels = (UINT)-1;
	}
	else if (dim == TextureDimension::Cubemap)
	{
		if (arraySize > 1)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.NumCubes = arraySize / 6;
			desc.TextureCubeArray.MipLevels = (UINT)-1;
		}
		else
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = (UINT)-1;
		}
		
	}
	return SUCCEEDED(g_render.device->CreateShaderResourceView(res, &desc, &dxSRV));
}

bool CreateTextureUAVImpl(UnorderedAccessView_t uav, Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	auto& dxUav = AllocUAV(uav);

	ID3D11Resource* res = Dx11_GetTexture(tex);
	if (!res)
		return true;

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};

	desc.Format = Dx11_Format(format);

	if (arraySize > 1)
	{
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = (UINT)arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}

	return SUCCEEDED(g_render.device->CreateUnorderedAccessView(res, &desc, &dxUav));
}

bool CreateTextureRTVImpl(RenderTargetView_t rtv, Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	auto& dxRtv = AllocRTV(rtv);

	ID3D11Resource* res = Dx11_GetTexture(tex);
	if (!res)
		return true;

	D3D11_RENDER_TARGET_VIEW_DESC desc = {};

	desc.Format = Dx11_Format(format);

	if(arraySize > 1)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = (UINT)arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipSlice = 0;
	}


	return SUCCEEDED(g_render.device->CreateRenderTargetView(res, &desc, &dxRtv));
}

bool CreateTextureDSVImpl(DepthStencilView_t dsv, Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	auto& dxDsv = AllocDSV(dsv);

	ID3D11Resource* res = Dx11_GetTexture(tex);
	if (!res)
		return true;

	D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};

	desc.Format = Dx11_Format(format);

	if (arraySize > 1)
	{
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = (UINT)arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}

	return SUCCEEDED(g_render.device->CreateDepthStencilView(res, &desc, &dxDsv));
}

bool CreateStructuredBufferSRVImpl(ShaderResourceView_t srv, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements)
{
	ID3D11Resource* res = Dx11_GetStructuredBuffer(buf);
	if (!res)
		return false;

	auto& dxSrv = AllocSRV(srv);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};

	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	desc.Buffer.FirstElement = (UINT)firstElement;
	desc.Buffer.NumElements = (UINT)numElements;

	return SUCCEEDED(g_render.device->CreateShaderResourceView(res, &desc, &dxSrv));
}

bool CreateStructuredBufferUAVImpl(UnorderedAccessView_t uav, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements)
{
	ID3D11Resource* res = Dx11_GetStructuredBuffer(buf);
	if (!res)
		return false;

	auto& dxUav = AllocUAV(uav);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};

	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	desc.Buffer.FirstElement = (UINT)firstElement;
	desc.Buffer.NumElements = (UINT)numElements;
	desc.Buffer.Flags = 0u;

	return SUCCEEDED(g_render.device->CreateUnorderedAccessView(res, &desc, &dxUav));
}

void DestroySRV(ShaderResourceView_t srv)
{
	if ((uint32_t)srv < g_SRVs.size())
		g_SRVs[(uint32_t)srv] = nullptr;
}

void DestroyUAV(UnorderedAccessView_t uav)
{
	if ((uint32_t)uav < g_UAVs.size())
		g_UAVs[(uint32_t)uav] = nullptr;
}

void DestroyRTV(RenderTargetView_t rtv)
{
	if ((uint32_t)rtv < g_RTVs.size())
		g_RTVs[(uint32_t)rtv] = nullptr;
}

void DestroyDSV(DepthStencilView_t dsv)
{
	if ((uint32_t)dsv < g_DSVs.size())
		g_DSVs[(uint32_t)dsv] = nullptr;
}

ID3D11ShaderResourceView* Dx11_GetShaderResourceView(ShaderResourceView_t srv)
{
	return (size_t)srv > 0 && (size_t)srv < g_SRVs.size() ? g_SRVs[(size_t)srv].Get() : nullptr;
}

ID3D11UnorderedAccessView* Dx11_GetUnorderedAccessView(UnorderedAccessView_t uav)
{
	return (size_t)uav > 0 && (size_t)uav < g_UAVs.size() ? g_UAVs[(size_t)uav].Get() : nullptr;
}

ID3D11RenderTargetView* Dx11_GetRenderTargetView(RenderTargetView_t rtv)
{
	return (size_t)rtv > 0 && (size_t)rtv < g_RTVs.size() ? g_RTVs[(size_t)rtv].Get() : nullptr;
}

ID3D11DepthStencilView* Dx11_GetDepthStencilView(DepthStencilView_t dsv)
{
	return (size_t)dsv > 0 && (size_t)dsv < g_DSVs.size() ? g_DSVs[(size_t)dsv].Get() : nullptr;
}

void DX11_CreateBackBufferRTV(RenderTargetView_t rtv, ID3D11Resource* backBufferResource)
{
	auto& dxRtv = AllocRTV(rtv);
	g_render.device->CreateRenderTargetView(backBufferResource, NULL, &dxRtv);
}