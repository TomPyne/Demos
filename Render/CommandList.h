#pragma once

#include "RenderTypes.h"
#include "PipelineState.h"

#include <memory>

FWD_RENDER_TYPE(ShaderResourceView_t);
FWD_RENDER_TYPE(RenderTargetView_t);
FWD_RENDER_TYPE(DepthStencilView_t);
FWD_RENDER_TYPE(UnorderedAccessView_t);
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
	void SetPipelineState(ComputePipelineState_t pso);
	void SetVertexBuffers(uint32_t startSlot, uint32_t count, const VertexBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets);
	void SetVertexBuffers(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets);
	void SetIndexBuffer(IndexBuffer_t ib, RenderFormat format, uint32_t indexOffset);
	void SetIndexBuffer(DynamicBuffer_t ib, RenderFormat format, uint32_t indexOffset);

	void CopyTexture(Texture_t dst, Texture_t src);

	void DrawIndexedInstanced(uint32_t numIndices, uint32_t numInstances, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance);
	void DrawInstanced(uint32_t numVerts, uint32_t numInstances, uint32_t startVertex, uint32_t startInstance);

	void Dispatch(uint32_t x, uint32_t y, uint32_t z);

	// Dx11 Style Bind Commands
	void BindVertexSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs);
	void BindVertexCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindVertexCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	void BindGeometryCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindGeometryCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	void BindPixelSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs);
	void BindPixelCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindPixelCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	void BindComputeSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs);
	void BindComputeUAVs(uint32_t startSlot, uint32_t count, const UnorderedAccessView_t* const srvs);
	void BindComputeCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs);
	void BindComputeCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs);

	GraphicsPipelineState_t GetPreviousPSO() const noexcept { return lastPipeline; }
	ComputePipelineState_t GetPreviousComputePSO() const noexcept { return lastComputePipeline; }

	static CommandListPtr Create();
	static void Execute(CommandListPtr& cl);
	static void ExecuteAndStall(CommandListPtr& cl);

	static void ReleaseAll();

public:

	// Helper functions and implementation macro for converting texture arrays into appropriate views.

#define BIND_HELPER_IMPL(FuncName, Type, GetFunc, SetFunc)			\
	template<uint32_t COUNT>										\
	void FuncName(uint32_t startSlot, Texture_t(&textures)[COUNT])	\
	{																\
		Type views[COUNT];											\
		for (uint32_t i = 0; i < COUNT; i++)						\
			views[i] = GetFunc(textures[i]);						\
		SetFunc(startSlot, COUNT, views);							\
	}																\

	BIND_HELPER_IMPL(BindTexturesAsVertexSRVs, ShaderResourceView_t, GetTextureSRV, BindVertexSRVs);
	BIND_HELPER_IMPL(BindTexturesAsPixelSRVs, ShaderResourceView_t, GetTextureSRV, BindPixelSRVs);
	BIND_HELPER_IMPL(BindTexturesAsComputeSRVs, ShaderResourceView_t, GetTextureSRV, BindComputeSRVs);
	BIND_HELPER_IMPL(BindTexturesAsComputeUAVs, UnorderedAccessView_t, GetTextureUAV, BindComputeUAVs);

private:
	std::unique_ptr<CommandListImpl> impl;

	GraphicsPipelineState_t lastPipeline = GraphicsPipelineState_t::INVALID;
	ComputePipelineState_t lastComputePipeline = ComputePipelineState_t::INVALID;

	void Begin();
	void Finish();
};