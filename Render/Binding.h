#pragma once

#include "RenderTypes.h"

RENDER_TYPE(ShaderResourceView_t);
RENDER_TYPE(UnorderedAccessView_t);
RENDER_TYPE(RenderTargetView_t);
RENDER_TYPE(DepthStencilView_t);

FWD_RENDER_TYPE(Texture_t);
FWD_RENDER_TYPE(StructuredBuffer_t);

enum class TextureDimension : uint8_t;

ShaderResourceView_t CreateTextureSRV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t arraySize);
UnorderedAccessView_t CreateTextureUAV(Texture_t tex, RenderFormat format, uint32_t arraySize);
RenderTargetView_t CreateTextureRTV(Texture_t tex, RenderFormat format, uint32_t arraySize);
DepthStencilView_t CreateTextureDSV(Texture_t tex, RenderFormat format, uint32_t arraySize);

ShaderResourceView_t CreateStructuredBufferSRV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems);
UnorderedAccessView_t CreateStructuredBufferUAV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems);

// This is a special case where we need to create an empty RTV so we can assign back buffer resources to it
RenderTargetView_t AllocTextureRTV(RenderFormat format, uint32_t arraySize);

RenderFormat GetSRVFormat(ShaderResourceView_t srv);
RenderFormat GetUAVFormat(UnorderedAccessView_t uav);
RenderFormat GetRTVFormat(RenderTargetView_t rtv);
RenderFormat GetDSVFormat(DepthStencilView_t dsv);

void ReleaseSRV(ShaderResourceView_t srv);
void ReleaseUAV(UnorderedAccessView_t uav);
void ReleaseRTV(RenderTargetView_t rtv);
void ReleaseDSV(DepthStencilView_t dsv);
