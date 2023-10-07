#pragma once

#include "../Textures.h"

bool CreateTextureImpl(Texture_t tex, const TextureCreateDescEx& desc);
bool UpdateTextureImpl(Texture_t tex, const void* const data, uint32_t width, uint32_t height, RenderFormat format);
void DestroyTexture(Texture_t tex);