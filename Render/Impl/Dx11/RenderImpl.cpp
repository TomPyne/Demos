#include "RenderImpl.h"
#include "../../Render.h"
#include "../../Buffers.h"

Dx11RenderGlobals g_render;

bool Render_Init()
{
	UINT createDeviceFlags = 0;
#if RENDER_DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL supportedFeatureLevels[1] = { D3D_FEATURE_LEVEL_11_1 };
	HRESULT hr = D3D11CreateDevice(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createDeviceFlags,
		supportedFeatureLevels,
		1,
		D3D11_SDK_VERSION,
		&g_render.device,
		&featureLevel,
		&g_render.context
	);

	if (FAILED(hr))
		return false;

	return true;
}

bool Render_Initialised()
{
	return g_render.device != nullptr;
}

void Render_NewFrame()
{
	DynamicBuffers_NewFrame();
}

void Render_ShutDown()
{
	g_render.context = nullptr;

	g_render.device = nullptr;	

	// RHI TODO: release all device resources if we shutdown the renderer and dont immediately close the program.
}

void Render_PushDebugWarningDisable(RenderDebugWarnings warning)
{
#if RENDER_DEBUG
	ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(g_render.device.As(&d3dDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
		{
			D3D11_MESSAGE_ID disable;
			switch (warning)
			{
			case RenderDebugWarnings::RENDER_TARGET_NOT_SET:
				disable = D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET;
				break;
			default:
				return;
			}

			UINT hh = d3dInfoQueue->GetStorageFilterStackSize();

			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = 1;
			filter.DenyList.pIDList = &disable;
			d3dInfoQueue->PushStorageFilter(&filter);
		}
	}
#endif
}

void Render_PopDebugWarningDisable()
{
#if RENDER_DEBUG
	ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(g_render.device.As(&d3dDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
		{
			UINT hh = d3dInfoQueue->GetStorageFilterStackSize();
			d3dInfoQueue->PopStorageFilter();
		}
	}
#endif
}
