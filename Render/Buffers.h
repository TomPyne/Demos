#pragma once

#include "RenderTypes.h"

RENDER_TYPE(VertexBuffer_t);
RENDER_TYPE(IndexBuffer_t);
RENDER_TYPE(StructuredBuffer_t);
RENDER_TYPE(ConstantBuffer_t);
RENDER_TYPE(DynamicBuffer_t);

VertexBuffer_t CreateVertexBuffer(const void* const data, size_t size);
IndexBuffer_t CreateIndexBuffer(const void* const data, size_t size);
StructuredBuffer_t CreateStructuredBuffer(const void* const data, size_t size, size_t stride, RenderResourceFlags flags);
ConstantBuffer_t CreateConstantBuffer(const void* const data, size_t size);

DynamicBuffer_t CreateDynamicVertexBuffer(const void* const data, size_t size);
DynamicBuffer_t CreateDynamicIndexBuffer(const void* const data, size_t size);
DynamicBuffer_t CreateDynamicConstantBuffer(const void* const data, size_t size);

void UpdateVertexBuffer(VertexBuffer_t vb, const void* const data, size_t size);
void UpdateIndexBuffer(IndexBuffer_t ib, const void* const data, size_t size);
void UpdateConstantBuffer(ConstantBuffer_t cb, const void* const data, size_t size);

void Render_Release(VertexBuffer_t vb);
void Render_Release(IndexBuffer_t ib);
void Render_Release(StructuredBuffer_t sb);
void Render_Release(ConstantBuffer_t cb);

void Render_Ref(VertexBuffer_t vb);
void Render_Ref(IndexBuffer_t ib);
void Render_Ref(StructuredBuffer_t sb);
void Render_Ref(ConstantBuffer_t cb);

void DynamicBuffers_NewFrame();