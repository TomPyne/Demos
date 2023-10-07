#include "../BuffersImpl.h"

#include "../../IDArray.h"
#include "RenderImpl.h"

#include <vector>

std::vector<ComPtr<ID3D11Buffer>> g_DxVertexBuffers;
std::vector<ComPtr<ID3D11Buffer>> g_DxIndexBuffers;
std::vector<ComPtr<ID3D11Buffer>> g_DxStructuredBuffers;
std::vector<ComPtr<ID3D11Buffer>> g_DxConstantBuffers;

std::vector<ComPtr<ID3D11Buffer>> g_DxDynamicBuffers;

static ComPtr<ID3D11Buffer>& AllocVertexBuffer(VertexBuffer_t vb)
{
	if ((size_t)vb >= g_DxVertexBuffers.size())
		g_DxVertexBuffers.resize((uint32_t)vb + 1);

	return g_DxVertexBuffers[(uint32_t)vb];
}

static ComPtr<ID3D11Buffer>& AllocIndexBuffer(IndexBuffer_t ib)
{
	if ((size_t)ib >= g_DxIndexBuffers.size())
		g_DxIndexBuffers.resize((uint32_t)ib + 1);

	return g_DxIndexBuffers[(uint32_t)ib];
}

static ComPtr<ID3D11Buffer>& AllocStructuredBuffer(StructuredBuffer_t sb)
{
	if ((size_t)sb >= g_DxStructuredBuffers.size())
		g_DxStructuredBuffers.resize((uint32_t)sb + 1);

	return g_DxStructuredBuffers[(uint32_t)sb];
}

static ComPtr<ID3D11Buffer>& AllocConstantBuffer(ConstantBuffer_t cb)
{
	if ((size_t)cb >= g_DxConstantBuffers.size())
		g_DxConstantBuffers.resize((uint32_t)cb + 1);

	return g_DxConstantBuffers[(uint32_t)cb];
}

bool CreateBuffer(const void* const data, UINT size, D3D11_USAGE usage, UINT bind, UINT misc, UINT stride, ComPtr<ID3D11Buffer>& buffer)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = size;
	desc.Usage = usage;
	desc.BindFlags = bind;
	desc.CPUAccessFlags = usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = misc;
	desc.StructureByteStride = stride;

	D3D11_SUBRESOURCE_DATA subRes = {};
	subRes.pSysMem = data;

	ComPtr<ID3D11Buffer> ret;

	return SUCCEEDED(g_render.device->CreateBuffer(&desc, data ? &subRes : nullptr, &buffer));
}

static bool CopyToBuffer(ID3D11Buffer* target, const void* const data, UINT size)
{
	if (!target)
		return false;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subRes = {};
	subRes.pSysMem = data;

	ComPtr<ID3D11Buffer> staging;

	if (FAILED(g_render.device->CreateBuffer(&desc, data ? &subRes : nullptr, &staging)))
		return false;

	g_render.context->CopySubresourceRegion(target, 0, 0, 0, 0, staging.Get(), 0, nullptr);

	return true;
}

bool CreateVertexBufferImpl(VertexBuffer_t handle, const void* const data, size_t size)
{
	return CreateBuffer(data, (UINT)size, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, AllocVertexBuffer(handle));
}

bool CreateIndexBufferImpl(IndexBuffer_t handle, const void* const data, size_t size)
{
	return CreateBuffer(data, (UINT)size, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, AllocIndexBuffer(handle));
}

bool CreateStructuredBufferImpl(StructuredBuffer_t handle, const void* const data, size_t size, size_t stride, RenderResourceFlags flags)
{
	return CreateBuffer(data, (UINT)size, D3D11_USAGE_DEFAULT, Dx11_BindFlags(flags), 0, (UINT)stride, AllocStructuredBuffer(handle));
}

bool CreateConstantBufferImpl(ConstantBuffer_t handle, const void* const data, size_t size)
{
	return CreateBuffer(data, (UINT)size, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, 0, 0, AllocConstantBuffer(handle));
}

void UpdateVertexBufferImpl(VertexBuffer_t vb, const void* const data, size_t size)
{
	CopyToBuffer(g_DxVertexBuffers[(uint32_t)vb].Get(), data, (UINT)size);
}

void UpdateIndexBufferImpl(IndexBuffer_t ib, const void* const data, size_t size)
{
	CopyToBuffer(g_DxIndexBuffers[(uint32_t)ib].Get(), data, (UINT)size);
}

void UpdateConstantBufferImpl(ConstantBuffer_t handle, const void* const data, size_t size)
{
	ID3D11Resource* res = g_DxConstantBuffers[(uint32_t)handle].Get();

	D3D11_MAPPED_SUBRESOURCE subRes;
	if (FAILED(g_render.context->Map(res, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes)))
		assert(0 && "UpdateConstantBufferImpl failed to map buffer");

	memcpy(subRes.pData, data, size);

	g_render.context->Unmap(res, 0);
}

void DestroyVertexBuffer(VertexBuffer_t handle)
{
	g_DxVertexBuffers[(uint32_t)handle] = nullptr;
}

void DestroyIndexBuffer(IndexBuffer_t handle)
{
	g_DxIndexBuffers[(uint32_t)handle] = nullptr;
}

void DestroyStructuredBuffer(StructuredBuffer_t handle)
{
	g_DxStructuredBuffers[(uint32_t)handle] = nullptr;
}

void DestroyConstantBuffer(ConstantBuffer_t handle)
{
	g_DxConstantBuffers[(uint32_t)handle] = nullptr;
}

ID3D11Buffer* Dx11_GetVertexBuffer(VertexBuffer_t vb)
{
	return g_DxVertexBuffers[(uint32_t)vb].Get();
}

ID3D11Buffer* Dx11_GetIndexBuffer(IndexBuffer_t ib)
{
	return g_DxIndexBuffers[(uint32_t)ib].Get();
}

ID3D11Buffer* Dx11_GetStructuredBuffer(StructuredBuffer_t sb)
{
	return g_DxStructuredBuffers[(uint32_t)sb].Get();
}

ID3D11Buffer* Dx11_GetConstantBuffer(ConstantBuffer_t cb)
{
	return g_DxConstantBuffers[(uint32_t)cb].Get();
}

ID3D11Buffer* Dx11_GetDynamicBuffer(DynamicBuffer_t db)
{
	return g_DxDynamicBuffers[(uint32_t)db].Get();
}

static DynamicBuffer_t CreateDynamicBuffer(const void* const data, size_t size, D3D11_BIND_FLAG bind)
{
	ComPtr<ID3D11Buffer> dynBuf;

	if (CreateBuffer(data, (UINT)size, D3D11_USAGE_IMMUTABLE, bind, 0, 0, dynBuf))
	{
		g_DxDynamicBuffers.push_back(dynBuf);
		return (DynamicBuffer_t)(g_DxDynamicBuffers.size() - 1);
	}
	
	return DynamicBuffer_t::INVALID;
}

DynamicBuffer_t CreateDynamicVertexBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, D3D11_BIND_VERTEX_BUFFER);
}

DynamicBuffer_t CreateDynamicIndexBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, D3D11_BIND_INDEX_BUFFER);
}

DynamicBuffer_t CreateDynamicConstantBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, D3D11_BIND_CONSTANT_BUFFER);
}

void DynamicBuffers_NewFrame()
{
	g_DxDynamicBuffers.resize(1);
}
