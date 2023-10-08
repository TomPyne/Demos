#include "DDSTextureLoader.h"

#include "Files.h"

#include <assert.h>
#include <algorithm>
#include <memory>

//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

//--------------------------------------------------------------------------------------
// DDS file structure definitions
//
// See DDS.h in the 'Texconv' sample and the 'DirectXTex' library
//--------------------------------------------------------------------------------------
#pragma pack(push,1)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
	uint32_t    size;
	uint32_t    flags;
	uint32_t    fourCC;
	uint32_t    RGBBitCount;
	uint32_t    RBitMask;
	uint32_t    GBitMask;
	uint32_t    BBitMask;
	uint32_t    ABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_DIMENSION_UNKNOWN 0
#define DDS_DIMENSION_BUFFER 1
#define DDS_DIMENSION_TEXTURE1D 2
#define DDS_DIMENSION_TEXTURE2D 3
#define DDS_DIMENSION_TEXTURE3D 4

#define DDS_MISC_FLAG_TEXTURE_CUBE 0x4L

enum DDS_MISC_FLAGS2
{
	DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

struct DDS_HEADER
{
	uint32_t        size;
	uint32_t        flags;
	uint32_t        height;
	uint32_t        width;
	uint32_t        pitchOrLinearSize;
	uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
	uint32_t        mipMapCount;
	uint32_t        reserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t        caps;
	uint32_t        caps2;
	uint32_t        caps3;
	uint32_t        caps4;
	uint32_t        reserved2;
};

struct DDS_HEADER_DXT10
{
	RenderFormat    format;
	uint32_t        resourceDimension;
	uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
	uint32_t        arraySize;
	uint32_t        miscFlags2;
};

#pragma pack(pop)

#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

RenderFormat GetRenderFormat(const DDS_PIXELFORMAT& ddpf) noexcept
{
	if (ddpf.flags & DDS_RGB)
	{
		// Note that sRGB formats are written using the "DX10" extended header

		switch (ddpf.RGBBitCount)
		{
		case 32:
			if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			{
				return RenderFormat::R8G8B8A8_UNORM;
			}

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			{
				return RenderFormat::B8G8R8A8_UNORM;
			}

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0))
			{
				return RenderFormat::B8G8R8X8_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0) aka D3DFMT_X8B8G8R8

			// Note that many common DDS reader/writers (including D3DX) swap the
			// the RED/BLUE masks for 10:10:10:2 formats. We assume
			// below that the 'backwards' header mask is being used since it is most
			// likely written by D3DX. The more robust solution is to use the 'DX10'
			// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

			// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
			if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
			{
				return RenderFormat::R10G10B10A2_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

			if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0))
			{
				return RenderFormat::R16G16_UNORM;
			}

			if (ISBITMASK(0xffffffff, 0, 0, 0))
			{
				// Only 32-bit color channel format in D3D9 was R32F
				return RenderFormat::R32_FLOAT; // D3DX writes this out as a FourCC of 114
			}
			break;

		case 24:
			// No 24bpp DXGI formats aka D3DFMT_R8G8B8
			break;

		case 16:
			if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
			{
				return RenderFormat::B5G5R5A1_UNORM;
			}
			if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0))
			{
				return RenderFormat::B5G6R5_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0) aka D3DFMT_X1R5G5B5

			if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
			{
				return RenderFormat::B4G4R4A4_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0) aka D3DFMT_X4R4G4B4

			// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
			break;
		}
	}
	else if (ddpf.flags & DDS_LUMINANCE)
	{
		if (8 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0xff, 0, 0, 0))
			{
				return RenderFormat::R8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4

			if (ISBITMASK(0x00ff, 0, 0, 0xff00))
			{
				return RenderFormat::R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
			}
		}

		if (16 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0xffff, 0, 0, 0))
			{
				return RenderFormat::R16_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (ISBITMASK(0x00ff, 0, 0, 0xff00))
			{
				return RenderFormat::R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
		}
	}
	else if (ddpf.flags & DDS_ALPHA)
	{
		if (8 == ddpf.RGBBitCount)
		{
			return RenderFormat::A8_UNORM;
		}
	}
	else if (ddpf.flags & DDS_BUMPDUDV)
	{
		if (16 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0x00ff, 0xff00, 0, 0))
			{
				return RenderFormat::R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
			}
		}

		if (32 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			{
				return RenderFormat::R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0))
			{
				return RenderFormat::R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
		}

		// No DXGI format maps to DDPF_BUMPLUMINANCE aka D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
	}
	else if (ddpf.flags & DDS_FOURCC)
	{
		if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
		{
			return RenderFormat::BC1_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
		{
			return RenderFormat::BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
		{
			return RenderFormat::BC3_UNORM;
		}

		// While pre-multiplied alpha isn't directly supported by the DXGI formats,
		// they are basically the same as these BC formats so they can be mapped
		if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
		{
			return RenderFormat::BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
		{
			return RenderFormat::BC3_UNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
		{
			return RenderFormat::BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
		{
			return RenderFormat::BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
		{
			return RenderFormat::BC4_SNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
		{
			return RenderFormat::BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
		{
			return RenderFormat::BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
		{
			return RenderFormat::BC5_SNORM;
		}

		// BC6H and BC7 are written using the "DX10" extended header

		if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
		{
			return RenderFormat::R8G8_B8G8_UNORM;
		}
		if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
		{
			return RenderFormat::G8R8_G8B8_UNORM;
		}

		if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
		{
			return RenderFormat::YUY2;
		}

		// Check for D3DFORMAT enums being set here
		switch (ddpf.fourCC)
		{
		case 36: // D3DFMT_A16B16G16R16
			return RenderFormat::R16G16B16A16_UNORM;

		case 110: // D3DFMT_Q16W16V16U16
			return RenderFormat::R16G16B16A16_SNORM;

		case 111: // D3DFMT_R16F
			return RenderFormat::R16_FLOAT;

		case 112: // D3DFMT_G16R16F
			return RenderFormat::R16G16_FLOAT;

		case 113: // D3DFMT_A16B16G16R16F
			return RenderFormat::R16G16B16A16_FLOAT;

		case 114: // D3DFMT_R32F
			return RenderFormat::R32_FLOAT;

		case 115: // D3DFMT_G32R32F
			return RenderFormat::R32G32_FLOAT;

		case 116: // D3DFMT_A32B32G32R32F
			return RenderFormat::R32G32B32A32_FLOAT;

			// No DXGI format maps to D3DFMT_CxV8U8
		}
	}

	return RenderFormat::UNKNOWN;
}

#undef ISBITMASK

bool FillInitData(
	size_t width,
	size_t height,
	size_t depth,
	size_t mipCount,
	size_t arraySize,
	RenderFormat format,
	size_t maxsize,
	size_t bitSize,
	const uint8_t* bitData,
	size_t& twidth,
	size_t& theight,
	size_t& tdepth,
	size_t& skipMip,
	MipData* initData) noexcept
{
	if (!bitData || !initData)
	{
		return false;
	}

	skipMip = 0;
	twidth = 0;
	theight = 0;
	tdepth = 0;

	size_t NumBytes = 0;
	size_t RowBytes = 0;
	const uint8_t* pSrcBits = bitData;
	const uint8_t* pEndBits = bitData + bitSize;

	size_t index = 0;
	for (size_t j = 0; j < arraySize; j++)
	{
		size_t w = width;
		size_t h = height;
		size_t d = depth;
		for (size_t i = 0; i < mipCount; i++)
		{
			Textures_GetSurfaceInfo((uint32_t)w, (uint32_t)h, format, &NumBytes, &RowBytes, nullptr);

			if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
				return false; // Arithmetic overflow.

			if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
			{
				if (!twidth)
				{
					twidth = w;
					theight = h;
					tdepth = d;
				}

				assert(index < mipCount* arraySize);
				initData[index].data = pSrcBits;
				initData[index].rowPitch = static_cast<UINT>(RowBytes);
				initData[index].slicePitch = static_cast<UINT>(NumBytes);
				++index;
			}
			else if (!j)
			{
				// Count number of skipped mipmaps (first item only)
				++skipMip;
			}

			if (pSrcBits + (NumBytes * d) > pEndBits)
			{
				return false; // EOF
			}

			pSrcBits += NumBytes * d;

			w = w >> 1;
			h = h >> 1;
			d = d >> 1;
			if (w == 0)
			{
				w = 1;
			}
			if (h == 0)
			{
				h = 1;
			}
			if (d == 0)
			{
				d = 1;
			}
		}
	}

	return index > 0;
}

bool LoadTextureDataFromFile(const char* path, std::unique_ptr<uint8_t[]>& ddsData, const DDS_HEADER** header, const uint8_t** bitData, size_t* bitSize)
{
	if (!header || !bitData || !bitSize)
		return false;

	
	std::vector<uint8_t> fileData = LoadBinaryFile(path);
	size_t fileSize = fileData.size();
	uint8_t* data = fileData.data();

	if (!data)
		return false;

	ddsData = std::make_unique<uint8_t[]>(fileSize);
	memcpy(ddsData.get(), data, fileSize);

	if (fileSize < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
		return false;

	// DDS files always start with the same magic number ("DDS ")
	auto dwMagicNumber = *reinterpret_cast<const uint32_t*>(ddsData.get());
	if (dwMagicNumber != DDS_MAGIC)
		return false;

	auto hdr = reinterpret_cast<const DDS_HEADER*>(ddsData.get() + sizeof(uint32_t));

	// Verify header to validate DDS file
	if (hdr->size != sizeof(DDS_HEADER) || hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
		return false;

	// Check for DX10 extension
	bool bDXT10Header = false;
	if ((hdr->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
	{
		// Must be long enough for both headers and magic value
		if (fileSize < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
			return false;

		bDXT10Header = true;
	}

	// setup the pointers in the process request
	*header = hdr;
	auto offset = sizeof(uint32_t) + sizeof(DDS_HEADER) + (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
	*bitData = ddsData.get() + offset;
	*bitSize = fileSize - offset;

	return true;
}

Texture_t CreateTextureFromDDS(
	const DDS_HEADER* header,
	const uint8_t* bitData,
	size_t bitSize,
	size_t maxSize,
	RenderResourceFlags flags)
{
	TextureCreateDescEx desc = {};
	
	desc.width = header->width;
	desc.height = header->height;
	desc.depth = header->depth;
	desc.mipCount = header->mipMapCount == 0 ? 1 : header->mipMapCount;
	desc.arraySize = 1;

	RenderFormat format = RenderFormat::UNKNOWN;

	uint32_t resDim = DDS_DIMENSION_UNKNOWN;
	
	if ((header->ddspf.flags & DDS_FOURCC) &&
		(MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
	{
		auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>(reinterpret_cast<const uint8_t*>(header) + sizeof(DDS_HEADER));

		desc.arraySize = d3d10ext->arraySize;
		if (desc.arraySize == 0)
			return Texture_t::INVALID;

		switch (d3d10ext->format)
		{
		case RenderFormat::AI44:
		case RenderFormat::IA44:
		case RenderFormat::P8:
		case RenderFormat::A8P8:
			return Texture_t::INVALID; // Not supported
		default:
			if (Textures_BitsPerPixel(d3d10ext->format) == 0)
			{
				return Texture_t::INVALID; // Not supported
			}
		}

		format = d3d10ext->format;

		switch (d3d10ext->resourceDimension)
		{
		case DDS_DIMENSION_TEXTURE1D:
			// D3DX writes 1D textures with a fixed Height of 1
			if ((header->flags & DDS_HEIGHT) && desc.height != 1)
			{
				return Texture_t::INVALID;
			}
			desc.height = desc.depth = 1;
			break;

		case DDS_DIMENSION_TEXTURE2D:
			if (d3d10ext->miscFlag & DDS_MISC_FLAG_TEXTURE_CUBE)
			{
				desc.arraySize *= 6;
				desc.dimension = TextureDimension::Cubemap;
			}
			desc.depth = 1;
			break;

		case DDS_DIMENSION_TEXTURE3D:
			if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
			{
				return Texture_t::INVALID; // Invalid data
			}

			if (desc.arraySize > 1)
			{
				return Texture_t::INVALID; // Not supported
			}
			break;

		default:
			return Texture_t::INVALID; // Not supported
		}

		resDim = d3d10ext->resourceDimension;
	}
	else
	{
		format = GetRenderFormat(header->ddspf);

		if (format == RenderFormat::UNKNOWN)
		{
			return Texture_t::INVALID; // Not supported
		}

		if (header->flags & DDS_HEADER_FLAGS_VOLUME)
		{
			resDim = DDS_DIMENSION_TEXTURE3D;
		}
		else
		{
			if (header->caps2 & DDS_CUBEMAP)
			{
				// We require all six faces to be defined
				if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
				{
					return Texture_t::INVALID; // Not supported
				}

				desc.arraySize = 6;
				desc.dimension = TextureDimension::Cubemap;
			}

			desc.depth = 1;
			resDim = DDS_DIMENSION_TEXTURE2D;

			// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
		}

		assert(Textures_BitsPerPixel(format) != 0);
	}

	// Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
	if (desc.mipCount > 15)
	{
		return Texture_t::INVALID; // Not supported
	}

	switch (resDim)
	{
	case DDS_DIMENSION_TEXTURE1D:
		if ((desc.arraySize > 2048) ||
			(desc.width > 16384))
		{
			return Texture_t::INVALID; // Not supported
		}
		else
		{
			desc.dimension = TextureDimension::Tex1D;
		}
		break;

	case DDS_DIMENSION_TEXTURE2D:
		if (desc.dimension == TextureDimension::Cubemap)
		{
			// This is the right bound because we set arraySize to (NumCubes*6) above
			if ((desc.arraySize > 2048) ||
				(desc.width > 16384) ||
				(desc.height > 16384))
			{
				return Texture_t::INVALID; // Not supported
			}
		}
		else if ((desc.arraySize > 2048) ||
			(desc.width > 16384) ||
			(desc.height > 16384))
		{
			return Texture_t::INVALID; // Not supported
		}
		else
		{
			desc.dimension = TextureDimension::Tex2D;
		}
		break;

	case DDS_DIMENSION_TEXTURE3D:
		if ((desc.arraySize > 1) ||
			(desc.width > 2048) ||
			(desc.height > 2048) ||
			(desc.depth > 2048))
		{
			return Texture_t::INVALID; // Not supported
		}
		else
		{
			desc.dimension = TextureDimension::Tex3D;
		}
		break;

	default:
		return Texture_t::INVALID; // Not supported
	}

	desc.resourceFormat = format;
	desc.srvFormat = format;

	// Create the texture
	std::unique_ptr<MipData[]> initData(new (std::nothrow) MipData[desc.mipCount * desc.arraySize]);

	size_t skipMip = 0;
	size_t twidth = 0;
	size_t theight = 0;
	size_t tdepth = 0;

	if (!FillInitData(desc.width, desc.height, desc.depth, desc.mipCount, desc.arraySize, format, maxSize, bitSize, bitData, twidth, theight, tdepth, skipMip, initData.get()))
		return Texture_t::INVALID; // Failed to init data

	desc.data = initData.get();
	desc.flags = RenderResourceFlags::SRV;
	return CreateTextureEx(desc);
}

Texture_t DDSTextureLoader_Load(const char* path)
{
	if (!path)
		return Texture_t::INVALID;

	const DDS_HEADER* header = nullptr;
	const uint8_t* bitData = nullptr;
	size_t bitSize = 0;

	std::unique_ptr<uint8_t[]> ddsData;
	if(!LoadTextureDataFromFile(path, ddsData, &header, &bitData, &bitSize))
		return Texture_t::INVALID;

	return CreateTextureFromDDS(header, bitData, bitSize, 0, RenderResourceFlags::SRV);
}