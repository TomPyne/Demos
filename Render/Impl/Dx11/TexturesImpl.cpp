#include "../TexturesImpl.h"

#include "RenderImpl.h"

std::vector<ComPtr<ID3D11Resource>> g_DxTextures;

static ComPtr<ID3D11Resource>& AllocTexture2D(Texture_t tex)
{
	if ((size_t)tex >= g_DxTextures.size())
		g_DxTextures.resize((size_t)tex + 1);

	return g_DxTextures[(size_t)tex];
}

static UINT Dx11_CpuAccessFlag(TextureCPUAccess access)
{
	UINT flag = 0u;

	if ((access & TextureCPUAccess::Write) != TextureCPUAccess::None)
		flag |= D3D11_CPU_ACCESS_WRITE;

	if ((access & TextureCPUAccess::Read) != TextureCPUAccess::None)
		flag |= D3D11_CPU_ACCESS_READ;

	return flag;
}

bool CreateTextureImpl(Texture_t tex, const TextureCreateDescEx& desc)
{
	auto& dxTex = AllocTexture2D(tex);

	const UINT bindFlags = Dx11_BindFlags(desc.flags);
	const DXGI_FORMAT resourceFormat = Dx11_Format(desc.resourceFormat);
	const D3D11_USAGE usage = Dx11_Usage(desc.usage);
	const UINT cpuAccessFlags = Dx11_CpuAccessFlag(desc.cpuAccess);

	std::unique_ptr<D3D11_SUBRESOURCE_DATA[]> subRes = nullptr; 
	if (desc.data)
	{
		subRes = std::make_unique<D3D11_SUBRESOURCE_DATA[]>(desc.mipCount * desc.arraySize);
		for (size_t i = 0; i < desc.mipCount * desc.arraySize; i++)
		{
			subRes[i].pSysMem = desc.data[i].data;
			subRes[i].SysMemPitch = static_cast<UINT>(desc.data[i].rowPitch);
			subRes[i].SysMemSlicePitch = static_cast<UINT>(desc.data[i].slicePitch);
		}
	}

	switch (desc.dimension)
	{
	case TextureDimension::Tex1D:
	{
		D3D11_TEXTURE1D_DESC dxDesc;
		dxDesc.Width = static_cast<UINT>(desc.width);
		dxDesc.MipLevels = static_cast<UINT>(desc.mipCount);
		dxDesc.ArraySize = static_cast<UINT>(desc.arraySize);
		dxDesc.Format = resourceFormat;
		dxDesc.Usage = usage;
		dxDesc.BindFlags = bindFlags;
		dxDesc.CPUAccessFlags = cpuAccessFlags;
		dxDesc.MiscFlags = 0;

		ComPtr<ID3D11Texture1D> dxTex1D;
		if (FAILED(g_render.device->CreateTexture1D(&dxDesc, subRes.get(), &dxTex1D)))
			return false;

		dxTex = dxTex1D;
	}
	break;
	case TextureDimension::Tex2D:
	case TextureDimension::Cubemap:
	{
		D3D11_TEXTURE2D_DESC dxDesc;
		dxDesc.Width = static_cast<UINT>(desc.width);
		dxDesc.Height = static_cast<UINT>(desc.height);
		dxDesc.MipLevels = static_cast<UINT>(desc.mipCount);
		dxDesc.ArraySize = static_cast<UINT>(desc.arraySize);
		dxDesc.Format = resourceFormat;
		dxDesc.SampleDesc.Count = 1;
		dxDesc.SampleDesc.Quality = 0;
		dxDesc.Usage = usage;
		dxDesc.BindFlags = bindFlags;
		dxDesc.CPUAccessFlags = cpuAccessFlags;
		dxDesc.MiscFlags = desc.dimension == TextureDimension::Cubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

		ComPtr<ID3D11Texture2D> dxTex2D;
		if (FAILED(g_render.device->CreateTexture2D(&dxDesc, subRes.get(), &dxTex2D)))
			return false;

		dxTex = dxTex2D;
	}
	break;
	case TextureDimension::Tex3D:
	{
		D3D11_TEXTURE3D_DESC dxDesc;
		dxDesc.Width = static_cast<UINT>(desc.width);
		dxDesc.Height = static_cast<UINT>(desc.height);
		dxDesc.Depth = static_cast<UINT>(desc.depth);
		dxDesc.MipLevels = static_cast<UINT>(desc.mipCount);
		dxDesc.Format = resourceFormat;
		dxDesc.Usage = usage;
		dxDesc.BindFlags = bindFlags;
		dxDesc.CPUAccessFlags = cpuAccessFlags;
		dxDesc.MiscFlags = 0;

		ComPtr<ID3D11Texture3D> dxTex3D;
		if (FAILED(g_render.device->CreateTexture3D(&dxDesc, subRes.get(), &dxTex3D)))
			return false;

		dxTex = dxTex3D;
	}
	break;
	default:
		return false;
	};

	return true;
}

bool UpdateTextureImpl(Texture_t tex, const void* const data, uint32_t width, uint32_t height, RenderFormat format)
{
	if ((size_t)tex < g_DxTextures.size())
		return false;

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));

	td.Width = (UINT)width;
	td.Height = (UINT)height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = Dx11_Format(format);
	td.Usage = D3D11_USAGE_STAGING;

	D3D11_SUBRESOURCE_DATA subRes = {};
	subRes.pSysMem = data;
	Dx11_CalculatePitch(td.Format, td.Width, td.Height, &subRes.SysMemPitch, &subRes.SysMemSlicePitch);

	ComPtr<ID3D11Texture2D> stagingTex;
	if (!SUCCEEDED(g_render.device->CreateTexture2D(&td, &subRes, &stagingTex)))
		return false;

	g_render.context->CopyResource(g_DxTextures[(uint32_t)tex].Get(), stagingTex.Get());

	return true;
}

void DestroyTexture(Texture_t tex)
{
	g_DxTextures[(uint32_t)tex] = nullptr;
}

ID3D11Resource* Dx11_GetTexture(Texture_t tex)
{
	return (size_t)tex > 0 && (size_t)tex < g_DxTextures.size() ? g_DxTextures[(uint32_t)tex].Get() : nullptr;
}

TextureResourceAccessScope::TextureResourceAccessScope(Texture_t resource, TextureResourceAccessMethod method, uint32_t subResourceIndex)
	: mappedTex(resource)
	, subResIdx(subResourceIndex)
{
	ID3D11Resource* dxRes = g_DxTextures[(uint32_t)mappedTex].Get();

	if (!dxRes)
		return;

	D3D11_MAP mapType;
	if (method == TextureResourceAccessMethod::Read)
		mapType = D3D11_MAP_READ;
	else if (method == TextureResourceAccessMethod::Write)
		mapType = D3D11_MAP_WRITE;
	else if (method == TextureResourceAccessMethod::ReadWrite)
		mapType = D3D11_MAP_READ_WRITE;
	else
		return;

	D3D11_MAPPED_SUBRESOURCE mapped;
	if(FAILED(g_render.context->Map(dxRes, subResourceIndex, mapType, 0, &mapped)))
		return;

	ptr = mapped.pData;
	rowPitch = mapped.RowPitch;
	depthPitch = mapped.DepthPitch;
}

TextureResourceAccessScope::~TextureResourceAccessScope()
{
	if(ptr)
		g_render.context->Unmap(g_DxTextures[(uint32_t)mappedTex].Get(), subResIdx);
}
