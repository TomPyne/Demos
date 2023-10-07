#pragma once

#include "RenderTypes.h"
#include "PipelineState.h"

#include <memory>

FWD_RENDER_TYPE(ShaderResourceView_t);
FWD_RENDER_TYPE(RenderTargetView_t);
FWD_RENDER_TYPE(DepthStencilView_t);
FWD_RENDER_TYPE(ConstantBuffer_t);
FWD_RENDER_TYPE(IndexBuffer_t);
FWD_RENDER_TYPE(VertexBuffer_t);
FWD_RENDER_TYPE(DynamicBuffer_t);
FWD_RENDER_TYPE(Texture_t);

struct CommandListImpl;

typedef std::shared_ptr<struct CommandList> CommandListPtr;

struct CommandList
{
	CommandList() = delete;
	CommandList(CommandListImpl* cl); // Takes ownership of pointer.
	CommandList(const CommandList&) = delete;
	~CommandList();

	void ClearRenderTarget(RenderTargetView_t rtv, const float col[4]);
	void ClearDepth(DepthStencilView_t dsv, float depth);

	void SetRenderTargets(const RenderTargetView_t* const rtvs, size_t num, DepthStencilView_t dsv);
	void SetViewports(const Viewport* const vp, size_t num);
	void SetDefaultScissor();
	void SetScissors(const ScissorRect* const scissors, size_t num);
	void SetPipelineState(GraphicsPipelineState_t pso);
	void SetVertexBuffers(uint32_t startSlot, uint32_t count, const VertexBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets);
	void SetVertexBuffers(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets);
	void SetIndexBuffer(IndexBuffer_t ib, RenderFormat format, uint32_t indexOffset);
	void SetIndexBuffer(DynamicBuffer_t ib, RenderFormat format, uint32_t indexOffset);

	void CopyTexture(Texture_t dst, Texture_t src);

	void DrawIndexedInstanced(uint32_t numIndices, uint32_t numInstances, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance);
	void DrawInstanced(uint32_t numVerts, uint32_t numInstances, uint32_t startVertex, uint32_t startInstance);

	// Dx11 Style Bind Commands
	void BindVertexSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs);
	void BindVertexTextures(uint32_t startSlot, uint32_t count, const Texture_t* const textures);
	void BindVertexCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindVertexCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	void BindGeometryCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindGeometryCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	void BindPixelSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs);
	void BindPixelTextures(uint32_t startSlot, uint32_t count, const Texture_t* const textures);
	void BindPixelCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindPixelCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	GraphicsPipelineState_t GetPreviousPSO() const noexcept { return lastPipeline; }

	static CommandListPtr Create();
	static void Execute(CommandListPtr& cl);
	static void ExecuteAndStall(CommandListPtr& cl);

	static void ReleaseAll();
	
private:
	std::unique_ptr<CommandListImpl> impl;
	GraphicsPipelineState_t lastPipeline = GraphicsPipelineState_t::INVALID;	

	void Begin();
	void Finish();
};