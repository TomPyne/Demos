#include "CommandList.h"

#include "Textures.h"

void CommandList::BindVertexTextures(uint32_t startSlot, uint32_t count, const Texture_t* const textures)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		ShaderResourceView_t srv = GetTextureSRV(textures[i]);
		BindVertexSRVs(slot, 1, &srv);
	}
}

void CommandList::BindPixelTextures(uint32_t startSlot, uint32_t count, const Texture_t* const textures)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		ShaderResourceView_t srv = GetTextureSRV(textures[i]);
		BindPixelSRVs(slot, 1, &srv);
	}
}