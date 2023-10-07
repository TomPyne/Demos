#pragma once

#include "RenderTypes.h"

enum class SamplerAddressMode : uint8_t
{
	Wrap,
	Mirror,
	Clamp,
	Border,
	MirrorOnce
};

static constexpr uint32_t kDefaultSamplerAddressMode = ((uint32_t)SamplerAddressMode::Wrap << 16) | ((uint32_t)SamplerAddressMode::Wrap << 8) | ((uint32_t)SamplerAddressMode::Wrap);

enum class SamplerFilterMode : uint8_t
{
	Point,
	Linear,
	Anisotropic
};

static constexpr uint32_t kDefaultSamplerFilterMode = ((uint32_t)SamplerFilterMode::Linear << 16) | ((uint32_t)SamplerFilterMode::Linear << 8) | ((uint32_t)SamplerFilterMode::Linear);

enum class SamplerComparisonFunc : uint8_t
{
	None,
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always
};

struct SamplerDesc
{
	union
	{
		struct
		{
			SamplerAddressMode AddressU;
			SamplerAddressMode AddressV;
			SamplerAddressMode AddressW;
		};
		uint32_t AddressMode = kDefaultSamplerAddressMode;
	};

	union
	{
		struct
		{
			SamplerFilterMode Min;
			SamplerFilterMode Mag;
			SamplerFilterMode Mip;
		};
		uint32_t FilterMode = kDefaultSamplerFilterMode;
	};

	SamplerComparisonFunc comparison = SamplerComparisonFunc::None;
	float minLOD = 0.0f;
	float maxLOD = 0.0f;
	float mipLODBias = 0.0f;
	float borderColor[4];
	uint32_t maxAnisotropy = 0;

	inline SamplerDesc& AddressModeUVW(SamplerAddressMode am)
	{
		AddressU = AddressV = AddressW = am; return *this;		
	}

	inline SamplerDesc& FilterModeMinMagMip(SamplerFilterMode fm)
	{
		Min = Mag = Mip = fm; return *this;
	}

	inline SamplerDesc& ComparisonFunc(SamplerComparisonFunc cf)
	{
		comparison = cf; return *this;
	}

	inline SamplerDesc& BorderColorRGBA(float col)
	{
		borderColor[0] = borderColor[1] = borderColor[2] = borderColor[3] = col; return *this;
	}
};

void InitSamplers(const SamplerDesc* const descs, size_t count);

