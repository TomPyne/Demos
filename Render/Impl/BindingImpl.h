#pragma once

#include "../Binding.h"

enum class TextureDimension : uint8_t;

bool CreateTextureSRVImpl(ShaderResourceView_t srv, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t arraySize);
bool CreateTextureUAVImpl(UnorderedAccessView_t uav, Texture_t tex, RenderFormat format, uint32_t arraySize);
bool CreateTextureRTVImpl(RenderTargetView_t rtv, Texture_t tex, RenderFormat format, uint32_t arraySize);
bool CreateTextureDSVImpl(DepthStencilView_t dsv, Texture_t tex, RenderFormat format, uint32_t arraySize);

bool CreateStructuredBufferSRVImpl(ShaderResourceView_t srv, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements);
bool CreateStructuredBufferUAVImpl(UnorderedAccessView_t uav, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements);

void DestroySRV(ShaderResourceView_t srv);
void DestroyUAV(UnorderedAccessView_t uav);
void DestroyRTV(RenderTargetView_t rtv);
void DestroyDSV(DepthStencilView_t dsv);