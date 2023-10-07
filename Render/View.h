#pragma once

#include "RenderTypes.h"
#include "Binding.h"

#include <memory>

struct CommandList;
struct RenderViewImpl;

struct RenderView
{
	explicit RenderView();
	~RenderView();

	void Resize(uint32_t x, uint32_t y);
	void Present(bool vsync);

	void ClearCurrentBackBufferTarget(CommandList* cl);

	RenderTargetView_t GetCurrentBackBufferRTV();

	intptr_t hwnd;
	std::unique_ptr<RenderViewImpl> impl;
	uint32_t width = 0;
	uint32_t height = 0;

	static constexpr RenderFormat BackBufferFormat = RenderFormat::R8G8B8A8_UNORM;
	static constexpr uint32_t NumBackBuffers = 2;
};

typedef std::shared_ptr<RenderView> RenderViewPtr;

RenderViewPtr CreateRenderViewPtr(intptr_t hwnd);
RenderView* CreateRenderView(intptr_t hwnd);
RenderView* GetRenderViewForHwnd(intptr_t hwnd);