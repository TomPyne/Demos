#include "TextureLoader.h"

#include "DDSTextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

void LazyCreateDefaultTex(Texture_t& tex, const uint32_t pix)
{
    if (tex == Texture_t::INVALID)
    {
        tex = TextureLoader_CreateTexture(&pix, 1u, 1u);
    }
    else
    {
        Render_AddRef(tex);
    }
}

Texture_t TextureLoader_WhiteTexture()
{
    static Texture_t whiteTex;

    LazyCreateDefaultTex(whiteTex, 0xFFFFFFFF);

    return whiteTex;
}

Texture_t TextureLoader_PinkTexture()
{
    static Texture_t pinkTex;

    LazyCreateDefaultTex(pinkTex, 0xFFC0CBFF);

    return pinkTex;
}

Texture_t TextureLoader_BlackTexture()
{
    static Texture_t blackTex;

    LazyCreateDefaultTex(blackTex, 0x00000000);

    return blackTex;
}

Texture_t TextureLoader_LoadTexture(const char* path)
{
    uint32_t w, h;
    return TextureLoader_LoadTexture(path, &w, &h);
}

Texture_t TextureLoader_LoadTexture(const char* path, uint32_t* w, uint32_t* h)
{
    int x, y, channels;
    unsigned char* pImageData = stbi_load(path, &x, &y, &channels, 4);

    *w = x;
    *h = y;

    Texture_t tex = TextureLoader_CreateTexture(pImageData, x, y);

    stbi_image_free(pImageData);

    return tex;
}

Texture_t TextureLoader_LoadDDSTexture(const char* path)
{
    return DDSTextureLoader_Load(path);
}

Texture_t TextureLoader_LoadPngTextureFromMemory(const void* data, size_t size, uint32_t* w, uint32_t* h)
{
    int x, y, channels;
    unsigned char* pImageData = stbi_load_from_memory((stbi_uc*)data, (int)size, &x, &y, &channels, 4);

    *w = x;
    *h = y;

    Texture_t tex = TextureLoader_CreateTexture(pImageData, x, y);

    stbi_image_free(pImageData);

    return tex;
}

Texture_t TextureLoader_CreateTexture(const void* data, uint32_t width, uint32_t height)
{
    TextureCreateDesc desc = {};
    desc.width = width;
    desc.height = height;
    desc.flags = RenderResourceFlags::SRV;
    desc.format = RenderFormat::R8G8B8A8_UNORM;

    MipData mip{data, desc.format, width, height};

    desc.data = &mip;

    return CreateTexture(desc);
}

void TextureLoader_UpdateTexture(Texture_t tex, const void* data, uint32_t width, uint32_t height)
{
    UpdateTexture(tex, data, width, height, RenderFormat::R8G8B8A8_UNORM);
}