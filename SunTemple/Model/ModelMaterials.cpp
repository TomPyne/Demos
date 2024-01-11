#include "ModelMaterials.h"

void ModelMaterials_InitPipelines()
{
	const char* shaderPath = "Gltf Viewer/Mesh.hlsl";

	VertexShader_t vs = CreateVertexShader(shaderPath);
	PixelShader_t blendPs = CreatePixelShader(shaderPath);
	PixelShader_t maskPs = CreatePixelShader(shaderPath, { "ALPHA_MASK" });

	InputElementDesc inputDesc[] =
	{
		{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PerVertex, 0 },
		{"NORMAL", 0, RenderFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PerVertex, 0 },
		{"TANGENT", 0, RenderFormat::R32G32B32A32_FLOAT, 2, 0, InputClassification::PerVertex, 0 },
		{"TEXCOORD", 0, RenderFormat::R32G32_FLOAT, 3, 0, InputClassification::PerVertex, 0 },
		{"TEXCOORD", 1, RenderFormat::R32G32_FLOAT, 4, 0, InputClassification::PerVertex, 0 },
	};

	GraphicsPipelineStateDesc desc = {};
	desc.DepthDesc(true, ComparisionFunc::LessEqual);
	desc.numRenderTargets = 1;

	for (u8 doubleSided = 0; doubleSided <= 1; doubleSided++)
	{
		MaterialID curId;
		curId.doubleSided = doubleSided;

		desc.RasterizerDesc(PrimitiveTopologyType::Triangle, FillMode::Solid, doubleSided == 1 ? CullMode::None : CullMode::Back);

		desc.vs = vs;
		desc.ps = blendPs;

		curId.blendMode = 0; // opaque
		desc.blendMode[0].None();

		pipelines[curId.opaque] = CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));

		curId.blendMode = 1; // blend
		desc.blendMode[0].Default();

		pipelines[curId.opaque] = CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));
	}
}
