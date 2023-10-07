#pragma once

#include "Shaders.h"

#include "RenderTypes.h"

RENDER_TYPE(GraphicsPipelineState_t);
RENDER_TYPE(ComputePipelineState_t);

enum class PrimitiveTopologyType : uint8_t
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class FillMode : uint8_t
{
	Solid,
	Wireframe
};

enum class CullMode : uint8_t
{
	None,
	Front,
	Back
};

enum class BlendType : uint8_t
{
	Zero = 1,
	One = 2,
	SrcColor = 3,
	InvSrcColor = 4,
	SrcAlpha = 5,
	InvSrcAlpha = 6,
	DstAlpha = 7,
	InvDstAlpha = 8,
	DstColor = 9,
	InvDstColor = 10,
	SrcAlphaSat = 11,
	Count
};

enum class BlendOp : uint8_t
{
	Add = 1,
	Subtract = 2,
	RevSubtract = 3,
	Min = 4,
	Max = 5,
	Count
};

struct BlendMode
{
	union
	{
		struct
		{
			bool enabled : 1;
			BlendType srcBlend : 4;
			BlendType dstBlend : 4;
			BlendOp op : 3;
			BlendType srcBlendAlpha : 4;
			BlendType dstBlendAlpha : 4;
			BlendOp opAlpha : 3;
		};
		uint32_t opaque = 0;
	};

	BlendMode& Enabled(bool e) noexcept { enabled = e; return *this; }
	BlendMode& Blend(BlendType s, BlendOp o, BlendType d) noexcept { srcBlend = s; op = o; dstBlend = d; return *this; }
	BlendMode& BlendAlpha(BlendType s, BlendOp o, BlendType d) noexcept { srcBlendAlpha = s; opAlpha = o; dstBlendAlpha = d; return *this; }
	BlendMode& Default() noexcept { return Enabled(true).Blend(BlendType::SrcAlpha, BlendOp::Add, BlendType::InvSrcAlpha).BlendAlpha(BlendType::InvSrcAlpha, BlendOp::Add, BlendType::Zero); }
	BlendMode& Add() noexcept { return Enabled(true).Blend(BlendType::One, BlendOp::Add, BlendType::One).BlendAlpha(BlendType::One, BlendOp::Add, BlendType::One); }
	BlendMode& None() noexcept { return Default().Enabled(false); }
};

enum class ComparisionFunc : uint8_t
{
	Never = 1,
	Less = 2,
	Equal = 3,
	LessEqual = 4,
	Greater = 5,
	NotEqual = 6,
	GreaterEqual = 7,
	Always = 8
};

struct GraphicsPipelineStateDesc
{
	static constexpr uint32_t kMaxRenderTargets = 8u;

	// Rasterizer desc
	PrimitiveTopologyType primTopo = PrimitiveTopologyType::Undefined;
	FillMode fillMode = FillMode::Solid;
	CullMode cullMode = CullMode::Back;
	int depthBias = 0;
	float depthBiasClamp = 0.0f;
	float slopeScaleDepthBias = 0.0f;

	// Depth desc
	bool depthEnabled = false;
	ComparisionFunc depthCompare = ComparisionFunc::Never;

	// Blend Desc
	BlendMode blendMode[kMaxRenderTargets];
	uint8_t numRenderTargets = 1;

	VertexShader_t vs = VertexShader_t::INVALID;
	GeometryShader_t gs = GeometryShader_t::INVALID;
	PixelShader_t ps = PixelShader_t::INVALID;

	GraphicsPipelineStateDesc& RasterizerDesc(PrimitiveTopologyType pt, FillMode fm, CullMode cm, int bias = 0, float biasClamp = 0.0f, float slopeScaleBias = 0.0f)
	{ 
		primTopo = pt; 
		fillMode = fm; 
		cullMode = cm; 
		depthBias = bias;
		depthBiasClamp = biasClamp;
		slopeScaleDepthBias = slopeScaleBias;
		return *this; 
	}

	GraphicsPipelineStateDesc& DepthDesc(bool enabled, ComparisionFunc cf) 
	{
		depthEnabled = enabled; 
		depthCompare = cf;
		return *this; 
	}
};

struct ComputePipelineStateDesc
{
	ComputeShader_t cs = ComputeShader_t::INVALID;
};

GraphicsPipelineState_t CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs = nullptr, size_t inputCount = 0);
ComputePipelineState_t CreateComputePipelineState(const ComputePipelineStateDesc& desc);

void Render_Release(GraphicsPipelineState_t pso);
void Render_Release(ComputePipelineState_t pso);