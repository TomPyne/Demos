#include "../../View.h"

#include "RenderImpl.h"

#include "../../Binding.h"
#include "../../CommandList.h"

#include <dxgi1_2.h>
#include <map>

#ifdef _MSC_VER
#pragma comment(lib, "dxgi.lib")
#endif

std::map<intptr_t, RenderView*> g_views;

struct RenderViewImpl
{
	ComPtr<IDXGISwapChain1> swapChain;
	RenderTargetView_t RTV = RenderTargetView_t::INVALID;
	uint32_t frameId = 0;
};

RenderView::RenderView()
	: impl(std::make_unique<RenderViewImpl>())
{
}

RenderView::~RenderView()
{
	g_views.erase(hwnd);
}

void RenderView::Resize(uint32_t x, uint32_t y)
{
	x = x > 0 ? x : 1;
	y = y > 0 ? y : 1;

	if (width == x && height == y)
		return;

	width = x;
	height = y;

	ReleaseRTV(impl->RTV);
	impl->RTV = RenderTargetView_t::INVALID;

	CommandList::ReleaseAll();

	impl->swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);

	ComPtr<ID3D11Texture2D> backBuffer = nullptr;

	impl->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

	assert(backBuffer);

	impl->RTV = AllocTextureRTV(BackBufferFormat, 1);

	DX11_CreateBackBufferRTV(impl->RTV, backBuffer.Get());
}

void RenderView::Present(bool vsync)
{
	impl->swapChain->Present(vsync, 0);

	impl->frameId++;
}

RenderViewPtr CreateRenderViewPtr(intptr_t hwnd)
{
	return std::shared_ptr<RenderView>(CreateRenderView(hwnd));
}

RenderView* CreateRenderView(intptr_t hwnd)
{
	RenderView* rv = new RenderView;

	rv->hwnd = hwnd;

	DXGI_SWAP_CHAIN_DESC1 sd;
	ZeroMemory(&sd, sizeof(sd));

	sd.Width = 0;
	sd.Height = 0;
	sd.Format = Dx11_Format(RenderView::BackBufferFormat);
	sd.Stereo = FALSE;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = RenderView::NumBackBuffers;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	ComPtr<IDXGIFactory2> factory;
	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
		return nullptr;

	if (FAILED(factory->CreateSwapChainForHwnd(g_render.device.Get(), (HWND)hwnd, &sd, NULL, NULL, &rv->impl->swapChain)))
		return nullptr;

	g_views[hwnd] = rv;

	return rv;
}

RenderView* GetRenderViewForHwnd(intptr_t hwnd)
{
	auto iter = g_views.find(hwnd);
	return iter != g_views.end() ? iter->second : nullptr;
}

void RenderView::ClearCurrentBackBufferTarget(CommandList* cl)
{
	RenderTargetView_t rtv = GetCurrentBackBufferRTV();
	if (rtv != RenderTargetView_t::INVALID)
	{
		constexpr float clearCol[4] = { 0.2f, 0.2f, 0.6f, 1.0f };
		cl->ClearRenderTarget(rtv, clearCol);
	}
}

RenderTargetView_t RenderView::GetCurrentBackBufferRTV()
{
	return impl->RTV;
}
