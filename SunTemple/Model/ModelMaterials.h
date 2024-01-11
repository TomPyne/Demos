#pragma once

#include "Render/Render.h"
#include "Utils/SurfMath.h"

union MaterialID
{
	struct
	{
		u8 doubleSided : 1;
		u8 blendMode : 1;
		u8 unused : 6;
	};
	u8 opaque = 0;
};

GraphicsPipelineState_t pipelines[1u << (1u + 2u)];

struct MaterialInstance
{
	MaterialID pipeline;

	float4 baseColorFactor = float4{ 1.0f };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;

	Texture_t baseColorTexture = Texture_t::INVALID;
	Texture_t normalTexture = Texture_t::INVALID;
	Texture_t metallicRoughnessTexture = Texture_t::INVALID;

	u32 baseColorUv = 0;
	u32 normalUv = 0;
	u32 metallicRoughnessUv = 0;

	bool alphaMask = false;
	float alphaCutoff = 0.5f;
};

void ModelMaterials_InitPipelines();