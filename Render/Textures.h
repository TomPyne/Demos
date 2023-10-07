#pragma once

#include "RenderTypes.h"

RENDER_TYPE(Texture_t);

enum class TextureDimension : uint8_t
{
	Unknown,
	Tex1D,
	Tex2D,
	Tex3D,
	Cubemap
};

struct MipData
{
	size_t rowPitch = 0;
	size_t slicePitch = 0;
	const void* data = nullptr;

	MipData() = default;
	MipData(const void* data, RenderFormat format, uint32_t width, uint32_t height);
};

struct TextureCreateDesc
{
	uint32_t width				= 0;
	uint32_t height				= 0;
	RenderFormat format			= RenderFormat::UNKNOWN;
	RenderResourceFlags flags	= RenderResourceFlags::None;
	MipData* data				= nullptr;
};

enum class TextureCPUAccess : uint8_t
{
	None,
	Read = (1u << 0u),
	Write = (1u << 1u),
};
IMPLEMENT_FLAGS(TextureCPUAccess, uint8_t);

struct TextureCreateDescEx
{
	uint32_t width				= 0;
	uint32_t height				= 0;
	uint32_t depth				= 1;
	uint32_t arraySize			= 1;
	uint32_t mipCount			= 1;
	TextureDimension dimension	= TextureDimension::Unknown;
	RenderResourceFlags flags	= RenderResourceFlags::None;	
	ResourceUsage usage			= ResourceUsage::Default;
	TextureCPUAccess cpuAccess	= TextureCPUAccess::None;
	const MipData* data			= nullptr;	

	RenderFormat resourceFormat = RenderFormat::UNKNOWN;
	RenderFormat srvFormat		= RenderFormat::UNKNOWN;
	RenderFormat uavFormat		= RenderFormat::UNKNOWN;
	RenderFormat rtvFormat		= RenderFormat::UNKNOWN;
	RenderFormat dsvFormat		= RenderFormat::UNKNOWN;
};

Texture_t CreateTexture(const void* const data, RenderFormat format, uint32_t width, uint32_t height);
Texture_t CreateTexture(const TextureCreateDesc& desc);
Texture_t CreateTextureEx(const TextureCreateDescEx& desc);

// The params here are for validation to ensure we are copying the intended data.
void UpdateTexture(Texture_t tex, const void* const data, uint32_t width, uint32_t height, RenderFormat format);

void SetTextureName(Texture_t tex, const char* name);

void Render_AddRef(Texture_t tex);
void Render_Release(Texture_t tex);

FWD_RENDER_TYPE(ShaderResourceView_t);
FWD_RENDER_TYPE(UnorderedAccessView_t);
FWD_RENDER_TYPE(RenderTargetView_t);
FWD_RENDER_TYPE(DepthStencilView_t);

ShaderResourceView_t	GetTextureSRV(Texture_t tex);
UnorderedAccessView_t	GetTextureUAV(Texture_t tex);
RenderTargetView_t		GetTextureRTV(Texture_t tex);
DepthStencilView_t		GetTextureDSV(Texture_t tex);

void GetTextureDims(Texture_t tex, uint32_t* w, uint32_t* h);

size_t Textures_BitsPerPixel(RenderFormat format);
void Textures_CalculatePitch(RenderFormat format, uint32_t width, uint32_t height, size_t* rowPitch, size_t* slicePitch);
void Textures_GetSurfaceInfo(uint32_t width, uint32_t height, RenderFormat format, size_t* outNumBytes, size_t* outRowBytes = nullptr, size_t* outNumRows = nullptr);

enum class TextureResourceAccessMethod : uint32_t
{
	Read,
	Write,
	ReadWrite,
};

struct TextureResourceAccessScope
{
	void* ptr = nullptr;

	size_t rowPitch = 0;
	size_t depthPitch = 0;

	Texture_t mappedTex = Texture_t::INVALID;
	uint32_t subResIdx = 0;

	explicit TextureResourceAccessScope(Texture_t resource, TextureResourceAccessMethod method, uint32_t subResourceIndex);
	~TextureResourceAccessScope();
};