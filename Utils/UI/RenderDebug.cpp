#include "RenderDebug.h"

#include "Render/Render.h"
#include "ThirdParty/imgui/imgui.h"

void ImGui_RenderDebug()
{
	if (!ImGui::Begin("Render Debug"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::Text("Shader Resource Views: %d", Bindings_GetShaderResourceViewCount());
	ImGui::Text("Unordered Access Views: %d", Bindings_GetUnorderedAccessViewCount());
	ImGui::Text("Render Target Views: %d", Bindings_GetRenderTargetViewCount());
	ImGui::Text("Depth Stencil Views: %d", Bindings_GetDepthStencilViewCount());
	ImGui::Text("Vertex Buffers: %d", Buffers_GetVertexBufferCount());
	ImGui::Text("Index Buffers: %d", Buffers_GetIndexBufferCount());
	ImGui::Text("Structured Buffers: %d", Buffers_GetStructuredBufferCount());
	ImGui::Text("Constant Buffers: %d", Buffers_GetConstantBufferCount());
	ImGui::Text("Graphics Pipelines: %d", PipelineStates_GetGraphicsPipelineStateCount());
	ImGui::Text("Compute Pipelines: %d", PipelineStates_GetComputePipelineStateCount());
	ImGui::Text("Vertex Shaders: %d", Shaders_GetVertexShaderCount());
	ImGui::Text("Pixel Shaders: %d", Shaders_GetPixelShaderCount());
	ImGui::Text("Geometry Shaders: %d", Shaders_GetGeometryShaderCount());
	ImGui::Text("Compute Shaders: %d", Shaders_GetComputeShaderCount());
	ImGui::Text("Textures: %d", Texture_GetTextureCount());

	ImGui::End();
}
