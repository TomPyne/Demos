#include "../../CommandList.h"

#include "RenderImpl.h"

std::vector<CommandListPtr> g_FreeCommandLists;

struct CommandListImpl
{
	ComPtr<ID3D11DeviceContext> context = nullptr;
	ComPtr<ID3D11CommandList> commandList = nullptr;
};

CommandList::CommandList(CommandListImpl* cl)
{
	impl = std::unique_ptr<CommandListImpl>(cl);
}

CommandList::~CommandList()
{
}

void CommandList::Begin()
{
	impl->context->ClearState();
	const UINT samplerCount = (UINT)Dx11_GetSamplerCount();
	for (UINT s = 0; s < samplerCount; s++)
	{
		ID3D11SamplerState* ss = Dx11_GetSampler(s);

		impl->context->VSSetSamplers(s, 1, &ss);
		impl->context->CSSetSamplers(s, 1, &ss);
		impl->context->PSSetSamplers(s, 1, &ss);
	}
}

void CommandList::Finish()
{
	impl->context->FinishCommandList(FALSE, &impl->commandList);
	lastPipeline = GraphicsPipelineState_t::INVALID;
}

void CommandList::ClearRenderTarget(RenderTargetView_t rtv, const float col[4])
{
	ID3D11RenderTargetView* dxRtv = Dx11_GetRenderTargetView(rtv);
	impl->context->ClearRenderTargetView(dxRtv, col);
}

void CommandList::ClearDepth(DepthStencilView_t dsv, float depth)
{
	ID3D11DepthStencilView* dxDsv = Dx11_GetDepthStencilView(dsv);
	impl->context->ClearDepthStencilView(dxDsv, D3D11_CLEAR_DEPTH, depth, 0);
}

void CommandList::SetRenderTargets(const RenderTargetView_t* const rtvs, size_t num, DepthStencilView_t dsv)
{
	assert(num <= 8);

	ID3D11RenderTargetView* dxRtvs[8];

	for(size_t i = 0; i < num; i++)
		dxRtvs[i] = Dx11_GetRenderTargetView(rtvs[i]);

	ID3D11DepthStencilView* dxDsv = Dx11_GetDepthStencilView(dsv);

	impl->context->OMSetRenderTargets((UINT)num, dxRtvs, dxDsv);
}

void CommandList::SetViewports(const Viewport* const vps, size_t num)
{
	assert(num <= 8);

	D3D11_VIEWPORT dxVps[8];
	for (size_t i = 0; i < num; i++)
	{
		dxVps[i].TopLeftX	= vps[i].topLeftX;
		dxVps[i].TopLeftY	= vps[i].topLeftY;
		dxVps[i].Width		= vps[i].width;
		dxVps[i].Height		= vps[i].height;
		dxVps[i].MinDepth	= vps[i].minDepth;
		dxVps[i].MaxDepth	= vps[i].maxDepth;
	}

	impl->context->RSSetViewports((UINT)num, dxVps);
}

void CommandList::SetDefaultScissor()
{
	D3D11_RECT rect = CD3D11_RECT(0, 0, LONG_MAX, LONG_MAX);
	impl->context->RSSetScissorRects(1, &rect);
}

void CommandList::SetScissors(const ScissorRect* const scissors, size_t num)
{
	assert(num <= 8);
	D3D11_RECT dxRects[8];
	for (size_t i = 0; i < num; i++)
	{
		dxRects[i].left = scissors[i].left;
		dxRects[i].top = scissors[i].top;
		dxRects[i].right = scissors[i].right;
		dxRects[i].bottom = scissors[i].bottom;
	}

	impl->context->RSSetScissorRects((UINT)num, dxRects);
}

void CommandList::SetPipelineState(GraphicsPipelineState_t pso)
{
	if (pso == lastPipeline)
		return;

	Dx11GraphicsPipelineState* dxPso = Dx11_GetGraphicsPipelineState(pso);

	impl->context->IASetPrimitiveTopology(dxPso->pt);
	impl->context->IASetInputLayout(dxPso->il.Get());
	impl->context->OMSetDepthStencilState(dxPso->dss.Get(), 0);
	impl->context->RSSetState(dxPso->rs.Get());
	impl->context->OMSetBlendState(dxPso->bs.Get(), nullptr, 0xffffffff);
	impl->context->VSSetShader(Dx11_GetVertexShader(dxPso->vs), nullptr, 0);
	impl->context->GSSetShader(Dx11_GetGeometryShader(dxPso->gs), nullptr, 0);
	impl->context->PSSetShader(Dx11_GetPixelShader(dxPso->ps), nullptr, 0);

	lastPipeline = pso;
}

void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, const VertexBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		ID3D11Buffer* dxVb = Dx11_GetVertexBuffer(vbs[i]);
		impl->context->IASetVertexBuffers(slot, 1, &dxVb, (const UINT*)(&strides[i]), (const UINT*)(&offsets[i]));
	}
}

void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		ID3D11Buffer* dxVb = Dx11_GetDynamicBuffer(vbs[i]);
		impl->context->IASetVertexBuffers(slot, 1, &dxVb, (const UINT*)(&strides[i]), (const UINT*)(&offsets[i]));
	}
}

void CommandList::SetIndexBuffer(IndexBuffer_t ib, RenderFormat format, uint32_t indexOffset)
{
	ID3D11Buffer* dxIb = Dx11_GetIndexBuffer(ib);
	impl->context->IASetIndexBuffer(dxIb, Dx11_Format(format), (UINT)indexOffset);
}

void CommandList::SetIndexBuffer(DynamicBuffer_t ib, RenderFormat format, uint32_t indexOffset)
{
	ID3D11Buffer* dxIb = Dx11_GetDynamicBuffer(ib);
	impl->context->IASetIndexBuffer(dxIb, Dx11_Format(format), (UINT)indexOffset);
}

void CommandList::CopyTexture(Texture_t dst, Texture_t src)
{
	ID3D11Resource* dxDst = Dx11_GetTexture(dst);
	ID3D11Resource* dxSrc = Dx11_GetTexture(src);

	impl->context->CopyResource(dxDst, dxSrc);
}

void CommandList::DrawIndexedInstanced(uint32_t numIndices, uint32_t numInstances, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance)
{
	impl->context->DrawIndexedInstanced((UINT)numIndices, (UINT)numInstances, (UINT)startIndex, (UINT)startVertex, (UINT)startInstance);
}

void CommandList::DrawInstanced(uint32_t numVerts, uint32_t numInstances, uint32_t startVertex, uint32_t startInstance)
{
	impl->context->DrawInstanced((UINT)numVerts, (UINT)numInstances, (UINT)startVertex, (UINT)startInstance);
}

// Dx11 Style Bind Commands
void CommandList::BindVertexSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11ShaderResourceView* dxSrv = Dx11_GetShaderResourceView(srvs[i]);
		impl->context->VSSetShaderResources(slot, 1, &dxSrv);
	}
}

void CommandList::BindVertexCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->VSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindVertexCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->VSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindGeometryCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->GSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindGeometryCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->GSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindPixelSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11ShaderResourceView* dxSrv = Dx11_GetShaderResourceView(srvs[i]);
		impl->context->PSSetShaderResources(slot, 1, &dxSrv);
	}
}

void CommandList::BindPixelCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->PSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindPixelCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->PSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

CommandListPtr CommandList::Create()
{
	if (!g_FreeCommandLists.empty())
	{
		CommandListPtr cl = g_FreeCommandLists.back();
		g_FreeCommandLists.pop_back();

		cl->Begin();

		return cl;
	}
	else
	{
		CommandListImpl* impl = new CommandListImpl;

		g_render.device->CreateDeferredContext(0, &impl->context);

		CommandListPtr cl = std::make_shared<CommandList>(impl);

		cl->Begin();

		return cl;
	}
}

void CommandList::Execute(CommandListPtr& cl)
{
	assert(cl);

	cl->Finish();

	assert(cl->impl->commandList);

	g_render.context->ExecuteCommandList(cl->impl->commandList.Get(), FALSE);

	g_FreeCommandLists.push_back(cl);
}

void CommandList::ExecuteAndStall(CommandListPtr& cl)
{
	D3D11_QUERY_DESC qDesc = {};
	qDesc.Query = D3D11_QUERY_EVENT;

	ComPtr<ID3D11Query> dxQuery;

	g_render.device->CreateQuery(&qDesc, &dxQuery);

	cl->Finish();

	g_render.context->ExecuteCommandList(cl->impl->commandList.Get(), FALSE);

	g_render.context->End(dxQuery.Get());

	UINT32 queryData;
	while (g_render.context->GetData(dxQuery.Get(), &queryData, sizeof(queryData), 0) != S_OK)
		Sleep(5);
}

void CommandList::ReleaseAll()
{
	g_FreeCommandLists.clear();
}
