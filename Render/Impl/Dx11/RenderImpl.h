#pragma once

#include "../../RenderTypes.h"
#include "../../Shaders.h"
#include "Dx11Types.h"

FWD_RENDER_TYPE(VertexShader_t);
FWD_RENDER_TYPE(PixelShader_t);
FWD_RENDER_TYPE(GeometryShader_t);
FWD_RENDER_TYPE(ComputeShader_t);

FWD_RENDER_TYPE(VertexBuffer_t);
FWD_RENDER_TYPE(IndexBuffer_t);
FWD_RENDER_TYPE(StructuredBuffer_t);
FWD_RENDER_TYPE(ConstantBuffer_t);
FWD_RENDER_TYPE(DynamicBuffer_t);

FWD_RENDER_TYPE(ShaderResourceView_t);
FWD_RENDER_TYPE(UnorderedAccessView_t);
FWD_RENDER_TYPE(RenderTargetView_t);
FWD_RENDER_TYPE(DepthStencilView_t);

FWD_RENDER_TYPE(Texture_t);

FWD_RENDER_TYPE(GraphicsPipelineState_t);
FWD_RENDER_TYPE(ComputePipelineState_t);

struct Dx11RenderGlobals
{
	ComPtr<ID3D11Device> device = nullptr;
	ComPtr<ID3D11DeviceContext> context = nullptr;
};

extern Dx11RenderGlobals g_render;

struct Dx11GraphicsPipelineState
{
	D3D11_PRIMITIVE_TOPOLOGY		pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	ComPtr<ID3D11InputLayout>		il = nullptr;
	ComPtr<ID3D11DepthStencilState> dss = nullptr;
	ComPtr<ID3D11RasterizerState>	rs = nullptr;
	ComPtr<ID3D11BlendState>		bs = nullptr;
	VertexShader_t					vs = VertexShader_t::INVALID;
	GeometryShader_t				gs = GeometryShader_t::INVALID;
	PixelShader_t					ps = PixelShader_t::INVALID;
};

struct Dx11ComputePipelineState
{
	ComputeShader_t _cs;
};

Dx11GraphicsPipelineState* Dx11_GetGraphicsPipelineState(GraphicsPipelineState_t pso);
Dx11ComputePipelineState* Dx11_ComputePipelineState(ComputePipelineState_t pso);

ID3DBlob* Dx11_GetVertexShaderBlob(VertexShader_t handle);
ID3D11VertexShader* Dx11_GetVertexShader(VertexShader_t vs);
ID3D11PixelShader* Dx11_GetPixelShader(PixelShader_t ps);
ID3D11GeometryShader* Dx11_GetGeometryShader(GeometryShader_t gs);
ID3D11ComputeShader* Dx11_GetComputeShader(ComputeShader_t cs);

ID3D11Buffer* Dx11_GetVertexBuffer(VertexBuffer_t vb);
ID3D11Buffer* Dx11_GetIndexBuffer(IndexBuffer_t ib);
ID3D11Buffer* Dx11_GetStructuredBuffer(StructuredBuffer_t sb);
ID3D11Buffer* Dx11_GetConstantBuffer(ConstantBuffer_t cb);
ID3D11Buffer* Dx11_GetDynamicBuffer(DynamicBuffer_t db);

ID3D11ShaderResourceView* Dx11_GetShaderResourceView(ShaderResourceView_t srv);
ID3D11UnorderedAccessView* Dx11_GetUnorderedAccessView(UnorderedAccessView_t uav);
ID3D11RenderTargetView* Dx11_GetRenderTargetView(RenderTargetView_t rtv);
ID3D11DepthStencilView* Dx11_GetDepthStencilView(DepthStencilView_t dsv);

ID3D11Resource* Dx11_GetTexture(Texture_t tex);

size_t Dx11_GetSamplerCount();
ID3D11SamplerState* Dx11_GetSampler(size_t index);

DXGI_FORMAT Dx11_Format(RenderFormat format);
UINT Dx11_BindFlags(RenderResourceFlags flags);
void Dx11_CalculatePitch(DXGI_FORMAT format, UINT width, UINT height, UINT* rowPitch, UINT* slicePitch);

void DX11_CreateBackBufferRTV(RenderTargetView_t rtv, ID3D11Resource* backBufferResource);