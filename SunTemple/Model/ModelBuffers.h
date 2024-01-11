#pragma once

#include "Render/Render.h"

struct BindVertexBuffer
{
	VertexBuffer_t buf = VertexBuffer_t::INVALID;
	uint32_t stride = 0;
	uint32_t offset = 0;
};

struct BindIndexBuffer
{
	IndexBuffer_t buf = IndexBuffer_t::INVALID;
	RenderFormat format = RenderFormat::UNKNOWN;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct ModelBuffers
{
	BindVertexBuffer positionBuf;
	BindVertexBuffer normalBuf;
	BindVertexBuffer tangentBuf;
	BindVertexBuffer texcoordBufs[4];
	BindVertexBuffer colorBufs[4];
	BindVertexBuffer jointBufs[4];
	BindVertexBuffer weightBufs[4];
	BindIndexBuffer indexBuf;
};
