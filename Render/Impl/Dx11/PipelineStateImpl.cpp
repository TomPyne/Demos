#include "../PipelineStateImpl.h"

#include "RenderImpl.h"

// TODO: DX12 treats PSOs as unique objects, but in DX11 it is possible to hash and cache each of these state objects
std::vector<Dx11GraphicsPipelineState> g_graphicsPipelines;
std::vector<Dx11ComputePipelineState> g_computePipelines;

static Dx11GraphicsPipelineState* AllocGraphicsPipeline(GraphicsPipelineState_t pso)
{
	if ((size_t)pso >= g_graphicsPipelines.size())
		g_graphicsPipelines.resize((size_t)pso + 1);

	return &g_graphicsPipelines[(size_t)pso];
}

static Dx11ComputePipelineState* AllocComputePipeline(ComputePipelineState_t pso)
{
	if ((size_t)pso >= g_computePipelines.size())
		g_computePipelines.resize((size_t)pso + 1);

	return &g_computePipelines[(size_t)pso];
}

static D3D11_COMPARISON_FUNC GetComparisonFunc(ComparisionFunc f)
{
	switch (f)
	{
	case ComparisionFunc::Never:		return D3D11_COMPARISON_NEVER;
	case ComparisionFunc::Less:			return D3D11_COMPARISON_LESS;
	case ComparisionFunc::Equal:		return D3D11_COMPARISON_EQUAL;
	case ComparisionFunc::LessEqual:	return D3D11_COMPARISON_LESS_EQUAL;
	case ComparisionFunc::Greater:		return D3D11_COMPARISON_GREATER;
	case ComparisionFunc::NotEqual:		return D3D11_COMPARISON_NOT_EQUAL;
	case ComparisionFunc::GreaterEqual:	return D3D11_COMPARISON_GREATER_EQUAL;
	case ComparisionFunc::Always:		return D3D11_COMPARISON_ALWAYS;
	}

	assert(0 && "Unsupported comparison func");
	return (D3D11_COMPARISON_FUNC)0;
}

D3D11_DEPTH_STENCIL_DESC CreateDSS(bool depthEnabled, ComparisionFunc func)
{
	D3D11_DEPTH_STENCIL_DESC dss;
	ZeroMemory(&dss, sizeof(dss));

	dss.DepthEnable = depthEnabled;
	dss.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dss.DepthFunc = GetComparisonFunc(func);

	return dss;
}

D3D11_RASTERIZER_DESC CreateRS(FillMode fm, CullMode cm, int bias, float biasClamp, float slopeBias)
{
	D3D11_RASTERIZER_DESC rs;
	ZeroMemory(&rs, sizeof(rs));

	switch (fm)
	{
	case FillMode::Solid:
		rs.FillMode = D3D11_FILL_SOLID;
		break;
	case FillMode::Wireframe:
		rs.FillMode = D3D11_FILL_WIREFRAME;
		break;
	};

	switch (cm)
	{
	case CullMode::None:
		rs.CullMode = D3D11_CULL_NONE;
		break;
	case CullMode::Front:
		rs.CullMode = D3D11_CULL_FRONT;
		break;
	case CullMode::Back:
		rs.CullMode = D3D11_CULL_BACK;
		break;
	};

	rs.ScissorEnable = true;
	rs.DepthClipEnable = true;
	rs.DepthBias = bias;
	rs.DepthBiasClamp = biasClamp;
	rs.SlopeScaledDepthBias = slopeBias;
	return rs;
}

static D3D11_BLEND GetBlend(BlendType b)
{
	switch (b)
	{
	case BlendType::Zero:			return D3D11_BLEND_ZERO;
	case BlendType::One:			return D3D11_BLEND_ONE;
	case BlendType::SrcColor:		return D3D11_BLEND_SRC_COLOR;
	case BlendType::InvSrcColor:	return D3D11_BLEND_INV_SRC_COLOR;
	case BlendType::SrcAlpha:		return D3D11_BLEND_SRC_ALPHA;
	case BlendType::InvSrcAlpha:	return D3D11_BLEND_INV_SRC_ALPHA;
	case BlendType::DstAlpha:		return D3D11_BLEND_DEST_ALPHA;
	case BlendType::InvDstAlpha:	return D3D11_BLEND_INV_DEST_ALPHA;
	case BlendType::DstColor:		return D3D11_BLEND_DEST_COLOR;
	case BlendType::InvDstColor:	return D3D11_BLEND_INV_DEST_COLOR;
	case BlendType::SrcAlphaSat:	return D3D11_BLEND_SRC_ALPHA_SAT;
	}

	assert(0 && "Unsupported blend func");
	return (D3D11_BLEND)0;
}

static D3D11_BLEND_OP GetBlendOp(BlendOp o)
{
	switch (o)
	{
	case BlendOp::Add:			return D3D11_BLEND_OP_ADD;
	case BlendOp::Subtract:		return D3D11_BLEND_OP_SUBTRACT;
	case BlendOp::RevSubtract:	return D3D11_BLEND_OP_REV_SUBTRACT;
	case BlendOp::Min:			return D3D11_BLEND_OP_MIN;
	case BlendOp::Max:			return D3D11_BLEND_OP_MAX;
	}

	assert(0 && "Unsupported blend op");
	return (D3D11_BLEND_OP)0;
}

D3D11_BLEND_DESC CreateBS(const BlendMode* bm, uint8_t count)
{
	D3D11_BLEND_DESC bs;
	ZeroMemory(&bs, sizeof(bs));

	for (uint8_t i = 1; i < count; i++)
	{
		if (bm[i].opaque != bm[0].opaque)
		{
			bs.IndependentBlendEnable = true;
			break;
		}			
	}

	for (uint8_t i = 0; i < count; i++)
	{
		bs.RenderTarget[i].BlendEnable = bm[i].enabled;
		bs.RenderTarget[i].SrcBlend = GetBlend(bm[i].srcBlend);
		bs.RenderTarget[i].DestBlend = GetBlend(bm[i].dstBlend);
		bs.RenderTarget[i].BlendOp = GetBlendOp(bm[i].op);
		bs.RenderTarget[i].SrcBlendAlpha = GetBlend(bm[i].srcBlendAlpha);
		bs.RenderTarget[i].DestBlendAlpha = GetBlend(bm[i].dstBlendAlpha);
		bs.RenderTarget[i].BlendOpAlpha = GetBlendOp(bm[i].opAlpha);
		bs.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	return bs;
}

D3D11_PRIMITIVE_TOPOLOGY GetPrimTopo(PrimitiveTopologyType pt)
{
	switch (pt)
	{
	case PrimitiveTopologyType::Point:
		return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PrimitiveTopologyType::Line:
		return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PrimitiveTopologyType::Triangle:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	assert(0 && "Unsupported primitive topology");
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

bool CompileGraphicsPipelineState(GraphicsPipelineState_t handle, const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount)
{
	Dx11GraphicsPipelineState* pso = AllocGraphicsPipeline(handle);

	{
		pso->vs = desc.vs;
		pso->gs = desc.gs;
		pso->ps = desc.ps;

		pso->pt = GetPrimTopo(desc.primTopo);
	}
	
	{
		D3D11_DEPTH_STENCIL_DESC dss = CreateDSS(desc.depthEnabled, desc.depthCompare);
		if (FAILED(g_render.device->CreateDepthStencilState(&dss, &pso->dss)))
		{
			fprintf(stderr, "CompileGraphicsPipelineState failed to create depth state");
			return false;
		}			
	}

	{
		D3D11_RASTERIZER_DESC rs = CreateRS(desc.fillMode, desc.cullMode, desc.depthBias, desc.depthBiasClamp, desc.slopeScaleDepthBias);
		if (FAILED(g_render.device->CreateRasterizerState(&rs, &pso->rs)))
		{
			fprintf(stderr, "CompileGraphicsPipelineState failed to create raster state");
			return false;
		}			
	}

	{
		D3D11_BLEND_DESC bs = CreateBS(desc.blendMode, desc.numRenderTargets);
		if (FAILED(g_render.device->CreateBlendState(&bs, &pso->bs)))
		{
			fprintf(stderr, "CompileGraphicsPipelineState failed to create blend state");
			return false;
		}			
	}

	if (inputs != nullptr && inputCount && desc.vs != VertexShader_t::INVALID)
	{
		ID3DBlob* blob = Dx11_GetVertexShaderBlob(desc.vs);
		assert(blob != nullptr && "Failed creating input layout, vertex blob is null");

		std::vector<D3D11_INPUT_ELEMENT_DESC> dxLayout;
		dxLayout.resize(inputCount);

		for (size_t i = 0; i < inputCount; i++)
		{
			dxLayout[i].AlignedByteOffset = (UINT)inputs[i].alinedByteOffset;
			dxLayout[i].Format = Dx11_Format(inputs[i].format);
			dxLayout[i].InputSlot = (UINT)inputs[i].inputSlot;
			dxLayout[i].InstanceDataStepRate = (UINT)inputs[i].instanceDataStepRate;
			dxLayout[i].SemanticIndex = (UINT)inputs[i].semanticIndex;
			dxLayout[i].SemanticName = (LPCSTR)inputs[i].semanticName;

			switch (inputs[i].inputSlotClass)
			{
			case InputClassification::PerVertex:
				dxLayout[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				break;
			case InputClassification::PerInstance:
				dxLayout[i].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
				break;
			};
		}

		if (FAILED(g_render.device->CreateInputLayout(dxLayout.data(), (UINT)dxLayout.size(), blob->GetBufferPointer(), blob->GetBufferSize(), &pso->il)))
		{
			fprintf(stderr, "CompileGraphicsPipelineState failed to create input layout");
			return false;
		}				
	}

	return true;
}

bool CompileComputePipelineState(ComputePipelineState_t handle, const ComputePipelineStateDesc& desc)
{
	if (desc.cs == ComputeShader_t::INVALID)
		return false;

	AllocComputePipeline(handle)->_cs = desc.cs;

	return true;
}

Dx11GraphicsPipelineState* Dx11_GetGraphicsPipelineState(GraphicsPipelineState_t pso)
{
	return &g_graphicsPipelines[(uint32_t)pso];
}

Dx11ComputePipelineState* Dx11_ComputePipelineState(ComputePipelineState_t pso)
{
	return &g_computePipelines[(uint32_t)pso];
}

void DestroyGraphicsPipelineState(GraphicsPipelineState_t pso)
{
	g_graphicsPipelines[(uint32_t)pso] = {};
}

void DestroyComputePipelineState(ComputePipelineState_t pso)
{
	g_computePipelines[(uint32_t)pso] = {};
}
