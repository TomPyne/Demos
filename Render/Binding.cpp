#include "Binding.h"
#include "Textures.h"
#include "Impl/BindingImpl.h"
#include "IDArray.h"

enum class ViewResourceType : uint8_t
{
	Unknown = 0,
	Texture,
	StructuredBuffer
};

struct TextureViewData
{
	Texture_t handle;
	RenderFormat format;
	TextureViewData(Texture_t t, RenderFormat f)
		: handle(t)
		, format(f)
	{}
};

struct BufferViewData
{
	StructuredBuffer_t handle;
	uint32_t firstElem;
	uint32_t numElems;
	BufferViewData(StructuredBuffer_t b, uint32_t fe, uint32_t ne)
		: handle(b)
		, firstElem(fe)
		, numElems(ne)
	{}
};

struct ViewData
{
	ViewResourceType type = ViewResourceType::Unknown;
	union
	{
		TextureViewData texture;
		BufferViewData buffer;
	};

	ViewData()
		: type(ViewResourceType::Unknown)
	{}

	ViewData(Texture_t tex, RenderFormat format)
		: texture(tex, format)
		, type(ViewResourceType::Texture)
	{}

	ViewData(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
		: buffer(buf, firstElem, numElems)
		, type(ViewResourceType::StructuredBuffer)
	{}
};

IDArray<ShaderResourceView_t, ViewData> g_SRVs;
IDArray<UnorderedAccessView_t, ViewData> g_UAVs;
IDArray<RenderTargetView_t, ViewData> g_RTVs;
IDArray<DepthStencilView_t, ViewData> g_DSVs;

ShaderResourceView_t CreateTextureSRV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t arraySize)
{
	ShaderResourceView_t srv = g_SRVs.Create(ViewData(tex, format));

	if (!CreateTextureSRVImpl(srv, tex, format, dim, mipLevels, arraySize))
	{
		g_SRVs.Release(srv);
		return ShaderResourceView_t::INVALID;
	}

	return srv;
}

UnorderedAccessView_t CreateTextureUAV(Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	UnorderedAccessView_t uav = g_UAVs.Create(ViewData(tex, format));

	if (!CreateTextureUAVImpl(uav, tex, format, arraySize))
	{
		g_UAVs.Release(uav);
		return UnorderedAccessView_t::INVALID;
	}

	return uav;
}

RenderTargetView_t CreateTextureRTV(Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	RenderTargetView_t rtv = g_RTVs.Create(ViewData(tex, format));

	if (!CreateTextureRTVImpl(rtv, tex, format, arraySize))
	{
		g_RTVs.Release(rtv);
		return RenderTargetView_t::INVALID;
	}

	return rtv;
}

DepthStencilView_t CreateTextureDSV(Texture_t tex, RenderFormat format, uint32_t arraySize)
{
	DepthStencilView_t dsv = g_DSVs.Create(ViewData(tex, format));

	if (!CreateTextureDSVImpl(dsv, tex, format, arraySize))
	{
		g_DSVs.Release(dsv);
		return DepthStencilView_t::INVALID;
	}

	return dsv;
}

ShaderResourceView_t CreateStructuredBufferSRV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
{
	ShaderResourceView_t srv = g_SRVs.Create(ViewData(buf, firstElem, numElems));

	if (!CreateStructuredBufferSRVImpl(srv, buf, firstElem, numElems))
	{
		g_SRVs.Release(srv);
		return ShaderResourceView_t::INVALID;
	}

	return srv;
}

UnorderedAccessView_t CreateStructuredBufferUAV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
{
	UnorderedAccessView_t uav = g_UAVs.Create(ViewData(buf, firstElem, numElems));

	if (!CreateStructuredBufferUAVImpl(uav, buf, firstElem, numElems))
	{
		g_UAVs.Release(uav);
		return UnorderedAccessView_t::INVALID;
	}

	return uav;
}

static RenderFormat GetViewDataFormat(ViewData* data)
{
	if (data && data->type == ViewResourceType::Texture)
		return data->texture.format;

	return RenderFormat::UNKNOWN;
}

RenderTargetView_t AllocTextureRTV(RenderFormat format, uint32_t arraySize)
{
	RenderTargetView_t rtv = g_RTVs.Create(ViewData(Texture_t::INVALID, format));

	if (!CreateTextureRTVImpl(rtv, Texture_t::INVALID, format, arraySize))
	{
		g_RTVs.Release(rtv);
		return RenderTargetView_t::INVALID;
	}

	return rtv;
}

RenderFormat GetSRVFormat(ShaderResourceView_t srv)
{
	return GetViewDataFormat(g_SRVs.Get(srv));
}

RenderFormat GetUAVFormat(UnorderedAccessView_t uav)
{
	return GetViewDataFormat(g_UAVs.Get(uav));
}

RenderFormat GetRTVFormat(RenderTargetView_t rtv)
{
	return GetViewDataFormat(g_RTVs.Get(rtv));
}

RenderFormat GetDSVFormat(DepthStencilView_t dsv)
{
	return GetViewDataFormat(g_DSVs.Get(dsv));
}

void ReleaseSRV(ShaderResourceView_t srv)
{
	if (g_SRVs.Release(srv))
		DestroySRV(srv);
}

void ReleaseUAV(UnorderedAccessView_t uav)
{
	if (g_UAVs.Release(uav))
		DestroyUAV(uav);
}

void ReleaseRTV(RenderTargetView_t rtv)
{
	if (g_RTVs.Release(rtv))
		DestroyRTV(rtv);
}

void ReleaseDSV(DepthStencilView_t dsv)
{
	if (g_DSVs.Release(dsv))
		DestroyDSV(dsv);
}