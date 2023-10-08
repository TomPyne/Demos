#pragma once

#include "Render/Render.h"

Texture_t TextureLoader_WhiteTexture();
Texture_t TextureLoader_PinkTexture();
Texture_t TextureLoader_BlackTexture();
Texture_t TextureLoader_LoadTexture(const char* path);
Texture_t TextureLoader_LoadTexture(const char* path, uint32_t* w, uint32_t* h);
Texture_t TextureLoader_LoadDDSTexture(const char* path);
Texture_t TextureLoader_LoadPngTextureFromMemory(const void* data, size_t size, uint32_t* w, uint32_t* h);
Texture_t TextureLoader_CreateTexture(const void* data, uint32_t width, uint32_t height);
void TextureLoader_UpdateTexture(Texture_t tex, const void* data, uint32_t width, uint32_t height);