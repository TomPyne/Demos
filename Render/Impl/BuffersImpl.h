#pragma once

#include "../Buffers.h"

bool CreateVertexBufferImpl(VertexBuffer_t handle, const void* const data, size_t size);
bool CreateIndexBufferImpl(IndexBuffer_t handle, const void* const data, size_t size);
bool CreateStructuredBufferImpl(StructuredBuffer_t handle, const void* data, size_t size, size_t stride, RenderResourceFlags flags);
bool CreateConstantBufferImpl(ConstantBuffer_t handle, const void* const data, size_t size);

void UpdateVertexBufferImpl(VertexBuffer_t vb, const void* const data, size_t size);
void UpdateIndexBufferImpl(IndexBuffer_t ib, const void* const data, size_t size);
void UpdateConstantBufferImpl(ConstantBuffer_t cb, const void* const data, size_t size);

void DestroyVertexBuffer(VertexBuffer_t handle);
void DestroyIndexBuffer(IndexBuffer_t handle);
void DestroyStructuredBuffer(StructuredBuffer_t handle);
void DestroyConstantBuffer(ConstantBuffer_t handle);